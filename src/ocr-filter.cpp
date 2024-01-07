
#include <opencv2/imgproc.hpp>

#include <numeric>
#include <memory>
#include <exception>
#include <fstream>
#include <new>
#include <mutex>
#include <filesystem>

#include <QString>

#include <util/bmem.h>

#include <plugin-support.h>
#include "filter-data.h"
#include "obs-utils.h"
#include "consts.h"
#include "tesseract-ocr-utils.h"
#include "ocr-filter.h"

const char *ocr_filter_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("OCRFilter");
}

/**                   PROPERTIES                     */

// Change the type of enable_smoothing_modified to obs_property_modified_t
bool enable_smoothing_modified(obs_properties_t *props, obs_property_t *property,
			       obs_data_t *settings)
{
	// show the smoothing filter properties only if the smoothing filter is enabled
	bool enable_smoothing = obs_data_get_bool(settings, "enable_smoothing");
	obs_property_set_visible(obs_properties_get(props, "word_length"), enable_smoothing);
	obs_property_set_visible(obs_properties_get(props, "window_size"), enable_smoothing);
	UNUSED_PARAMETER(property);
	return true;
}

obs_properties_t *ocr_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	// Add page segmentation mode property
	obs_property_t *psm_list = obs_properties_add_list(props, "page_segmentation_mode",
							   obs_module_text("PageSegmentationMode"),
							   OBS_COMBO_TYPE_LIST,
							   OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(psm_list, "Fully automatic", (long long)tesseract::PSM_AUTO);
	obs_property_list_add_int(psm_list, "Single column",
				  (long long)tesseract::PSM_SINGLE_COLUMN);
	obs_property_list_add_int(psm_list, "Single block vertical",
				  (long long)tesseract::PSM_SINGLE_BLOCK_VERT_TEXT);
	obs_property_list_add_int(psm_list, "Single block", (long long)tesseract::PSM_SINGLE_BLOCK);
	obs_property_list_add_int(psm_list, "Single line", (long long)tesseract::PSM_SINGLE_LINE);
	obs_property_list_add_int(psm_list, "Single word", (long long)tesseract::PSM_SINGLE_WORD);
	obs_property_list_add_int(psm_list, "Circle word", (long long)tesseract::PSM_CIRCLE_WORD);
	obs_property_list_add_int(psm_list, "Single character",
				  (long long)tesseract::PSM_SINGLE_CHAR);
	obs_property_list_add_int(psm_list, "Sparse text", (long long)tesseract::PSM_SPARSE_TEXT);
	obs_property_list_add_int(psm_list, "Sparse text with orientation",
				  (long long)tesseract::PSM_SPARSE_TEXT_OSD);

	// Add language property, list selection from "eng" and "scoreboard"
	obs_property *lang_list =
		obs_properties_add_list(props, "language", obs_module_text("Language"),
					OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	// scan the tessdata folder for files using std::filesystem
	std::string tessdata_folder = obs_module_file("tessdata");
	obs_log(LOG_INFO, "Scanning tessdata folder: %s", tessdata_folder.c_str());
	std::vector<std::string> tessdata_files;
	for (const auto &entry : std::filesystem::directory_iterator(tessdata_folder)) {
		std::string filename = entry.path().filename().string();
		if (filename.find(".traineddata") != std::string::npos) {
			obs_log(LOG_INFO, "Found traineddata file: %s", filename.c_str());
			tessdata_files.push_back(filename);
		}
	}

	// Add the found traineddata files to the language list
	for (const auto &filename : tessdata_files) {
		std::string language = filename.substr(0, filename.find(".traineddata"));
		obs_property_list_add_string(lang_list, language.c_str(), language.c_str());
	}

	// Add character whitelist
	obs_properties_add_text(props, "char_whitelist", "Char whitelist", OBS_TEXT_DEFAULT);

	// Add user patterns multiline text input
	obs_properties_add_text(props, "user_patterns", "User patterns", OBS_TEXT_MULTILINE);

	// Add conf thershold property
	obs_properties_add_int_slider(props, "conf_threshold", obs_module_text("ConfThreshold"), 0,
				      100, 1);

	// Add property to enable or disable the smoothing filter
	obs_property_t *enable_smoothing_property = obs_properties_add_bool(
		props, "enable_smoothing", obs_module_text("EnableSmoothing"));
	obs_properties_add_int_slider(props, "word_length", obs_module_text("WordLength"), 1, 20,
				      1);
	obs_properties_add_int_slider(props, "window_size", obs_module_text("WindowSize"), 1, 20,
				      1);

	obs_property_set_modified_callback(enable_smoothing_property, enable_smoothing_modified);

	// Add a property for the output text source
	obs_property_t *sources = obs_properties_add_list(props, "text_sources",
							  "Output text source", OBS_COMBO_TYPE_LIST,
							  OBS_COMBO_FORMAT_STRING);

	// Add a "none" option
	obs_property_list_add_string(sources, "No output", "none");
	// Add the sources
	obs_enum_sources(add_sources_to_list, sources);

	// Add a informative text about the plugin
	obs_properties_add_text(
		props, "info",
		QString(PLUGIN_INFO_TEMPLATE).arg(PLUGIN_VERSION).toStdString().c_str(),
		OBS_TEXT_INFO);

	UNUSED_PARAMETER(data);
	return props;
}

void ocr_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "page_segmentation_mode", tesseract::PSM_SINGLE_WORD);
	obs_data_set_default_string(settings, "language", "eng");
	obs_data_set_default_string(settings, "text_sources", "none");
	obs_data_set_default_string(
		settings, "char_whitelist",
		"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.,:;!?-()[]{}<>|/@#$%&*+=_~");
	obs_data_set_default_int(settings, "conf_threshold", 50);
	obs_data_set_default_string(settings, "user_patterns", "");
	obs_data_set_default_bool(settings, "enable_smoothing", false);
	obs_data_set_default_int(settings, "word_length", 5);
	obs_data_set_default_int(settings, "window_size", 10);
}

void ocr_filter_update(void *data, obs_data_t *settings)
{
	struct filter_data *tf = reinterpret_cast<filter_data *>(data);

	// Update the output text source
	update_text_source_on_settings(tf, settings);

	tf->pageSegmentationMode = (int)obs_data_get_int(settings, "page_segmentation_mode");
	tf->language = obs_data_get_string(settings, "language");
	tf->char_whitelist = obs_data_get_string(settings, "char_whitelist");
	tf->user_patterns = obs_data_get_string(settings, "user_patterns");
	tf->conf_threshold = (int)obs_data_get_int(settings, "conf_threshold");
	tf->enable_smoothing = obs_data_get_bool(settings, "enable_smoothing");
	tf->word_length = obs_data_get_int(settings, "word_length");
	tf->window_size = obs_data_get_int(settings, "window_size");

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

	// get the models folder path from the module
	tf->tesseractTraineddataFilepath = obs_module_file("tessdata");

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

		if (tf->tesseractTraineddataFilepath != nullptr) {
			bfree(tf->tesseractTraineddataFilepath);
		}
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

static std::string processImageForOCR(struct filter_data *tf, const cv::Mat &imageBGRA)
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
		std::unique_lock<std::mutex> lock(tf->inputBGRALock, std::try_to_lock);
		if (!lock.owns_lock()) {
			return;
		}
		imageBGRA = tf->inputBGRA.clone();
	}

	try {
		// Process the image
		std::string ocr_result = processImageForOCR(tf, imageBGRA);

		if (ocr_result.empty()) {
			obs_log(LOG_DEBUG, "OCR failed to process image.");
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
