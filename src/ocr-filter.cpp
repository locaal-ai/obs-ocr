#ifdef _WIN32
#include <wchar.h>
#endif // _WIN32

#include <opencv2/imgproc.hpp>

#include <numeric>
#include <memory>
#include <exception>
#include <fstream>
#include <new>
#include <mutex>

#include <QString>

#include <plugin-support.h>
#include "filter-data.h"
#include "obs-utils.h"
#include "consts.h"
#include "tesseract-ocr-utils.h"

const char *ocr_filter_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("OCRFilter");
}

/**                   PROPERTIES                     */

obs_properties_t *ocr_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	// Add page segmentation mode property
	obs_property_t *psm_list = obs_properties_add_list(
		props, "page_segmentation_mode",
		obs_module_text("PageSegmentationMode"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(psm_list, "Fully automatic",
				  (long long)tesseract::PSM_AUTO);
	obs_property_list_add_int(psm_list, "Single column",
				  (long long)tesseract::PSM_SINGLE_COLUMN);
	obs_property_list_add_int(
		psm_list, "Single block vertical",
		(long long)tesseract::PSM_SINGLE_BLOCK_VERT_TEXT);
	obs_property_list_add_int(psm_list, "Single block",
				  (long long)tesseract::PSM_SINGLE_BLOCK);
	obs_property_list_add_int(psm_list, "Single line",
				  (long long)tesseract::PSM_SINGLE_LINE);
	obs_property_list_add_int(psm_list, "Single word",
				  (long long)tesseract::PSM_SINGLE_WORD);
	obs_property_list_add_int(psm_list, "Circle word",
				  (long long)tesseract::PSM_CIRCLE_WORD);
	obs_property_list_add_int(psm_list, "Single character",
				  (long long)tesseract::PSM_SINGLE_CHAR);
	obs_property_list_add_int(psm_list, "Sparse text",
				  (long long)tesseract::PSM_SPARSE_TEXT);
	obs_property_list_add_int(psm_list, "Sparse text with orientation",
				  (long long)tesseract::PSM_SPARSE_TEXT_OSD);

	// Add language property, list selection from "eng" and "scoreboard"
	obs_property *lang_list = obs_properties_add_list(
		props, "language", obs_module_text("Language"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(lang_list, obs_module_text("English"),
				     "eng");
	obs_property_list_add_string(lang_list, obs_module_text("Scoreboard"),
				     "scoreboard");

	// Add a property for the output text source
	obs_property_t *sources = obs_properties_add_list(props, "text_sources",
							  "Output text source", OBS_COMBO_TYPE_LIST,
							  OBS_COMBO_FORMAT_STRING);
	// Add a "none" option
	obs_property_list_add_string(sources, "No output", "none");
	// Add the sources
	obs_enum_sources(add_sources_to_list, sources);

	// Add a informative text about the plugin
	obs_properties_add_text(props, "info",
				QString(PLUGIN_INFO_TEMPLATE)
					.arg(PLUGIN_VERSION)
					.toStdString()
					.c_str(),
				OBS_TEXT_INFO);

	UNUSED_PARAMETER(data);
	return props;
}

void ocr_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "page_segmentation_mode",
				 tesseract::PSM_SINGLE_WORD);
	obs_data_set_default_string(settings, "language", "eng");
	obs_data_set_default_string(settings, "text_sources", "none");
}

void ocr_filter_update(void *data, obs_data_t *settings)
{
	struct filter_data *tf = reinterpret_cast<filter_data *>(data);

	// Update the output text source
	update_text_source_on_settings(tf, settings);

	// Get the page segmentation mode
	tf->pageSegmentationMode =
		(int)obs_data_get_int(settings, "page_segmentation_mode");

	// Get the language
	tf->language = obs_data_get_string(settings, "language");

	// Initialize the Tesseract OCR model
	initialize_tesseract_ocr(tf);
}

void ocr_filter_activate(void *data)
{
	struct filter_data *tf = reinterpret_cast<filter_data *>(data);
	tf->isDisabled = false;
}

void ocr_filter_deactivate(void *data)
{
	struct filter_data *tf = reinterpret_cast<filter_data *>(data);
	tf->isDisabled = true;
}

/**                   FILTER CORE                     */

void *ocr_filter_create(obs_data_t *settings, obs_source_t *source)
{
	void *data = bmalloc(sizeof(struct filter_data));
	struct filter_data *tf = new (data) filter_data();

	tf->source = source;
	tf->texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);
	tf->output_source_name = bstrdup(obs_data_get_string(settings, "text_sources"));
	tf->output_source = nullptr;
	// initialize the mutex
	tf->output_source_mutex = new std::mutex();

	// get the model file path from the module
#if _WIN32
	tf->modelFilepath = obs_module_file("crnn_cs.onnx");
	tf->tesseractTraineddataFilepath = obs_module_file("tessdata");
	tf->vocabularyFilepath = obs_module_file("alphabet_36.txt");
#else
	tf->modelFilepath = obs_module_file("crnn_cs.onnx");
	tf->tesseractTraineddataFilepath = obs_module_file("tessdata");
	tf->vocabularyFilepath = obs_module_file("alphabet_94.txt");
#endif

	tf->tesseract_model = nullptr;

	ocr_filter_update(tf, settings);

	return tf;
}

void ocr_filter_destroy(void *data)
{
	struct filter_data *tf = reinterpret_cast<filter_data *>(data);

	if (tf) {
		obs_enter_graphics();
		gs_texrender_destroy(tf->texrender);
		if (tf->stagesurface) {
			gs_stagesurface_destroy(tf->stagesurface);
		}
		obs_leave_graphics();

		if (tf->tesseract_model != nullptr) {
			tf->tesseract_model->End();
			delete tf->tesseract_model;
			tf->tesseract_model = nullptr;
		}
		if (tf->output_source_mutex) {
			delete tf->output_source_mutex;
			tf->output_source_mutex = nullptr;
		}

		tf->~filter_data();
		bfree(tf);
	}
}

static std::string processImageForOCR(struct filter_data *tf,
				      const cv::Mat &imageBGRA)
{
	std::string recognitionResult = run_tesseract_ocr(tf, imageBGRA);

	return recognitionResult;
}

void ocr_filter_video_tick(void *data, float seconds)
{
	struct filter_data *tf = reinterpret_cast<filter_data *>(data);

	if (tf->isDisabled) {
		return;
	}

	if (!obs_source_enabled(tf->source)) {
		return;
	}

	if (tf->inputBGRA.empty()) {
		return;
	}

	cv::Mat imageBGRA;
	{
		std::unique_lock<std::mutex> lock(tf->inputBGRALock,
						  std::try_to_lock);
		if (!lock.owns_lock()) {
			return;
		}
		imageBGRA = tf->inputBGRA.clone();
	}

	try {
		// Process the image
		std::string ocr_result = processImageForOCR(tf, imageBGRA);

		if (ocr_result.empty()) {
			// Something went wrong.
			obs_log(LOG_WARNING, "OCR failed to process image.");
			return;
		}

		if (is_valid_output_source_name(tf->output_source_name)) {
			// If an output source is selected - send the results there
			setTextCallback(ocr_result, tf);
		}
	} catch (const std::exception &e) {
		obs_log(LOG_ERROR, "%s", e.what());
		return;
	}

	UNUSED_PARAMETER(seconds);
}

void ocr_filter_video_render(void *data, gs_effect_t *_effect)
{
	UNUSED_PARAMETER(_effect);

	struct filter_data *tf = reinterpret_cast<filter_data *>(data);

	if (tf->isDisabled) {
		if (tf->source) {
			obs_source_skip_video_filter(tf->source);
		}
		return;
	}

	uint32_t width, height;
	if (!getRGBAFromStageSurface(tf, width, height)) {
		if (tf->source) {
			obs_source_skip_video_filter(tf->source);
		}
		return;
	}

	obs_source_skip_video_filter(tf->source);
}

struct obs_source_info ocr_filter_info = {
	.id = "ocr_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = ocr_filter_getname,
	.create = ocr_filter_create,
	.destroy = ocr_filter_destroy,
	.get_defaults = ocr_filter_defaults,
	.get_properties = ocr_filter_properties,
	.update = ocr_filter_update,
	.activate = ocr_filter_activate,
	.deactivate = ocr_filter_deactivate,
	.video_tick = ocr_filter_video_tick,
	.video_render = ocr_filter_video_render,
};
