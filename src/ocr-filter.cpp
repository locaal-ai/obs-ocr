
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

bool update_on_change_modified(obs_properties_t *props, obs_property_t *property,
			       obs_data_t *settings)
{
	bool update_on_change = obs_data_get_bool(settings, "update_on_change");
	obs_property_set_visible(obs_properties_get(props, "update_on_change_threshold"),
				 update_on_change);
	UNUSED_PARAMETER(property);
	return true;
}

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

bool binarization_mode_modified(obs_properties_t *props, obs_property_t *property,
				obs_data_t *settings)
{
	long long binMode = obs_data_get_int(settings, "binarization_mode");
	obs_property_set_visible(obs_properties_get(props, "binarization_threshold"), binMode == 1);
	obs_property_set_visible(obs_properties_get(props, "binarization_block_size"),
				 binMode == 2 || binMode == 3);
	obs_property_set_visible(obs_properties_get(props, "preview_binarization"), binMode != 0);
	UNUSED_PARAMETER(property);
	return true;
}

bool rescale_modified(obs_properties_t *props_modified, obs_property_t *property,
		      obs_data_t *settings)
{
	bool rescale_image = obs_data_get_bool(settings, "rescale_image");
	obs_property_set_visible(obs_properties_get(props_modified, "rescale_target_size"),
				 rescale_image);
	UNUSED_PARAMETER(property);
	return true;
}

obs_properties_t *ocr_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	// Add language property, list selection from "eng" and "scoreboard"
	obs_property *lang_list =
		obs_properties_add_list(props, "language", obs_module_text("Language"),
					OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	// scan the tessdata folder for files using std::filesystem
	std::string tessdata_folder = obs_module_file("tessdata");
	obs_log(LOG_DEBUG, "Scanning tessdata folder: %s", tessdata_folder.c_str());
	for (const auto &entry : std::filesystem::directory_iterator(tessdata_folder)) {
		std::string filename = entry.path().filename().string();
		if (filename.find(".traineddata") != std::string::npos) {
			obs_log(LOG_DEBUG, "Found traineddata file: %s", filename.c_str());
			std::string language = filename.substr(0, filename.find(".traineddata"));
			obs_property_list_add_string(lang_list, language.c_str(), language.c_str());
		}
	}

	// Add update timer property
	obs_properties_add_int(props, "update_timer", obs_module_text("UpdateTimer"), 1, 100000, 1);

	// add advanced settings checkbox
	obs_properties_add_bool(props, "advanced_settings", obs_module_text("AdvancedSettings"));

	// Add property for "update on change" checkbox
	obs_properties_add_bool(props, "update_on_change", obs_module_text("UpdateOnChange"));
	// Add update threshold property
	obs_properties_add_int_slider(props, "update_on_change_threshold",
				      obs_module_text("UpdateOnChangeThreshold"), 1, 100, 1);
	// Add a callback to enable or disable the update threshold property
	obs_property_set_modified_callback(obs_properties_get(props, "update_on_change"),
					   update_on_change_modified);

	obs_property_set_modified_callback(
		obs_properties_get(props, "advanced_settings"),
		[](obs_properties_t *props_modified, obs_property_t *property,
		   obs_data_t *settings) {
			bool advanced_settings = obs_data_get_bool(settings, "advanced_settings");
			for (const char *prop :
			     {"page_segmentation_mode", "char_whitelist", "conf_threshold",
			      "user_patterns", "enable_smoothing", "word_length", "window_size",
			      "update_on_change", "binarization_mode", "preview_binarization",
			      "binarization_threshold", "binarization_block_size", "rescale_image",
			      "rescale_target_size", "update_on_change_threshold",
			      "dilation_iterations", "output_flatten", "char_whitelist_preset",
			      "current_output"}) {
				obs_property_set_visible(obs_properties_get(props_modified, prop),
							 advanced_settings);
			}
			if (advanced_settings) {
				enable_smoothing_modified(props_modified, nullptr, settings);
				update_on_change_modified(props_modified, nullptr, settings);
				binarization_mode_modified(props_modified, nullptr, settings);
				rescale_modified(props_modified, nullptr, settings);
			}
			UNUSED_PARAMETER(property);
			return true;
		});

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

	// Add binarization options dropdown list
	obs_property_t *binarization_list = obs_properties_add_list(
		props, "binarization_mode", obs_module_text("BinarizationMode"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(binarization_list, "No binarization", (long long)0);
	obs_property_list_add_int(binarization_list, "Threshold", (long long)1);
	obs_property_list_add_int(binarization_list, "Adaptive mean", (long long)2);
	obs_property_list_add_int(binarization_list, "Adaptive gaussian", (long long)3);
	obs_property_list_add_int(binarization_list, "Triangle", (long long)4);
	obs_property_list_add_int(binarization_list, "Otsu", (long long)5);

	// add threshold parameter for binarization
	obs_properties_add_int_slider(props, "binarization_threshold",
				      obs_module_text("BinarizationThreshold"), 0, 255, 1);

	// add adaptive theshold block size
	obs_properties_add_int_slider(props, "binarization_block_size",
				      obs_module_text("BinarizationBlockSize"), 3, 255, 2);

	// add callback to enable or disable the binarization threshold and block size properties
	obs_property_set_modified_callback(obs_properties_get(props, "binarization_mode"),
					   binarization_mode_modified);

	// add dilation iterations
	obs_properties_add_int_slider(props, "dilation_iterations",
				      obs_module_text("DilationIterations"), 0, 10, 1);

	// Add option for previewing the binarization
	obs_properties_add_bool(props, "preview_binarization",
				obs_module_text("PreviewBinarization"));

	// add option for rescaling the image
	obs_properties_add_bool(props, "rescale_image", obs_module_text("RescaleImage"));

	// add rescale target size slider
	obs_properties_add_int_slider(props, "rescale_target_size",
				      obs_module_text("RescaleTargetSize"), 10, 100, 1);

	// add callback to enable or disable the rescale target size property
	obs_property_set_modified_callback(
		obs_properties_get(props, "rescale_image"),
		[](obs_properties_t *props_modified, obs_property_t *property,
		   obs_data_t *settings) {
			bool rescale_image = obs_data_get_bool(settings, "rescale_image");
			obs_property_set_visible(obs_properties_get(props_modified,
								    "rescale_target_size"),
						 rescale_image);
			UNUSED_PARAMETER(property);
			return true;
		});

	// add preset selector for char whitelist
	obs_property_t *char_whitelist_preset = obs_properties_add_list(
		props, "char_whitelist_preset", obs_module_text("CharWhitelistPreset"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(char_whitelist_preset, obs_module_text("SelectPresets"),
				     "none");
	obs_property_list_add_string(char_whitelist_preset, obs_module_text("English"),
				     WHITELIST_CHARS_ENGLISH);
	obs_property_list_add_string(char_whitelist_preset, obs_module_text("NumericPunctuation"),
				     WHITELIST_CHARS_NUMERIC);
	obs_property_list_add_string(char_whitelist_preset, obs_module_text("French"),
				     WHITELIST_CHARS_FRENCH);
	obs_property_list_add_string(char_whitelist_preset, obs_module_text("German"),
				     WHITELIST_CHARS_GERMAN);
	obs_property_list_add_string(char_whitelist_preset, obs_module_text("Spanish"),
				     WHITELIST_CHARS_SPANISH);
	obs_property_list_add_string(char_whitelist_preset, obs_module_text("Italian"),
				     WHITELIST_CHARS_ITALIAN);
	obs_property_list_add_string(char_whitelist_preset, obs_module_text("Portuguese"),
				     WHITELIST_CHARS_PORTUGUESE);
	obs_property_list_add_string(char_whitelist_preset, obs_module_text("Cyrillic"),
				     WHITELIST_CHARS_RUSSIAN);
	obs_property_list_add_string(char_whitelist_preset, obs_module_text("Japanese"),
				     WHITELIST_CHARS_JAPANESE);
	obs_property_list_add_string(char_whitelist_preset, obs_module_text("Hindi"),
				     WHITELIST_CHARS_HINDI);
	obs_property_list_add_string(char_whitelist_preset, obs_module_text("Arabic"),
				     WHITELIST_CHARS_ARABIC);
	// add callback to set the char whitelist based on the selected preset
	obs_property_set_modified_callback(
		obs_properties_get(props, "char_whitelist_preset"),
		[](obs_properties_t *, obs_property_t *property, obs_data_t *settings) {
			const char *selected_preset =
				obs_data_get_string(settings, "char_whitelist_preset");
			if (strcmp(selected_preset, "none") != 0) {
				obs_data_set_string(settings, "char_whitelist", selected_preset);
			}
			UNUSED_PARAMETER(property);
			return true;
		});

	// Add character whitelist
	obs_properties_add_text(props, "char_whitelist", obs_module_text("CharWhitelist"),
				OBS_TEXT_DEFAULT);

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

	// Output formatting
	obs_properties_add_text(props, "output_formatting", obs_module_text("OutputFormatting"),
				OBS_TEXT_MULTILINE);
	// hide the output formatting property by default
	obs_property_set_visible(obs_properties_get(props, "output_formatting"), false);

	// add option to "flatten" the output text to a single line
	obs_properties_add_bool(props, "output_flatten", obs_module_text("OutputFlatten"));

	// Add a property for the output text source
	obs_property_t *text_sources =
		obs_properties_add_list(props, "text_sources", obs_module_text("OutputTextSource"),
					OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_enum_sources(
		[](void *data, obs_source_t *source) -> bool {
			obs_property_t *text_sources = (obs_property_t *)data;
			const char *source_type = obs_source_get_id(source);
			if (strstr(source_type, "text") != NULL) {
				const char *name = obs_source_get_name(source);
				obs_property_list_add_string(text_sources, name, name);
			}

			return true;
		},
		text_sources);

	// Add a "none" option
	obs_property_list_add_string(text_sources, obs_module_text("NoOutput"), "none");
	// Add a "save to file" option
	obs_property_list_add_string(text_sources, obs_module_text("SaveToFile"),
				     "!!save_to_file!!");

	// Add an option to set the output file path
	obs_properties_add_path(props, "output_file_path", obs_module_text("OutputFilePath"),
				OBS_PATH_FILE, nullptr, nullptr);
	// add an option to control output file aggegation / "append" mode
	obs_properties_add_bool(props, "output_file_append", obs_module_text("OutputFileAppend"));

	// add callback to enable or disable the output file path property
	obs_property_set_modified_callback(
		obs_properties_get(props, "text_sources"),
		[](obs_properties_t *props_modified, obs_property_t *property,
		   obs_data_t *settings) {
			bool save_to_file = strcmp(obs_data_get_string(settings, "text_sources"),
						   "!!save_to_file!!") == 0;
			obs_property_set_visible(obs_properties_get(props_modified,
								    "output_file_path"),
						 save_to_file);
			obs_property_set_visible(obs_properties_get(props_modified,
								    "output_file_append"),
						 save_to_file);
			// show/hide "output_formatting" property based on the selected output source
			bool show_output_formatting =
				strcmp(obs_data_get_string(settings, "text_sources"), "none") != 0;
			obs_property_set_visible(obs_properties_get(props_modified,
								    "output_formatting"),
						 show_output_formatting);
			UNUSED_PARAMETER(property);
			return true;
		});

	// Add a property for the output text detection mask image source
	obs_property_t *image_sources =
		obs_properties_add_list(props, "text_detection_mask_sources",
					obs_module_text("OutputTextDetectionMaskSource"),
					OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	// Add a "none" option
	obs_property_list_add_string(image_sources, obs_module_text("NoOutput"), "none");
	// Add the sources
	obs_enum_sources(add_image_sources_to_list, image_sources);
	// add change callback for the image sources
	obs_property_set_modified_callback(
		obs_properties_get(props, "text_detection_mask_sources"),
		[](obs_properties_t *props_modified, obs_property_t *, obs_data_t *settings) {
			// hide/show the image_output_option property based on the selected image source
			bool show_image_output_option =
				strcmp(obs_data_get_string(settings, "text_detection_mask_sources"),
				       "none") != 0;
			obs_property_set_visible(obs_properties_get(props_modified,
								    "image_output_option"),
						 show_image_output_option);
			return true;
		});

	// add a choice for the image output format: detection boxes mask, text rendering, or text with background
	obs_property_t *output_format = obs_properties_add_list(
		props, "image_output_option", obs_module_text("ImageOutputOption"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(output_format, obs_module_text("DetectionBoxesMask"),
				  OUTPUT_IMAGE_OPTION_DETECTION_MASK);
	obs_property_list_add_int(output_format, obs_module_text("TextRendering"),
				  OUTPUT_IMAGE_OPTION_TEXT_OVERLAY);
	obs_property_list_add_int(output_format, obs_module_text("TextWithBackground"),
				  OUTPUT_IMAGE_OPTION_TEXT_BACKGROUND);

	// add current output text box, disabled by default
	obs_properties_add_text(props, "current_output", obs_module_text("current_output"),
				OBS_TEXT_DEFAULT);
	obs_property_set_enabled(obs_properties_get(props, "current_output"), false);

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
	obs_data_set_default_int(settings, "update_timer", 100);
	obs_data_set_default_bool(settings, "update_on_change", true);
	obs_data_set_default_int(settings, "update_on_change_threshold", 15);
	obs_data_set_default_string(settings, "language", "eng");
	obs_data_set_default_bool(settings, "advanced_settings", false);
	obs_data_set_default_int(settings, "page_segmentation_mode", tesseract::PSM_AUTO);
	obs_data_set_default_int(settings, "binarization_mode", 0);
	obs_data_set_default_int(settings, "binarization_threshold", 127);
	obs_data_set_default_int(settings, "binarization_block_size", 15);
	obs_data_set_default_bool(settings, "preview_binarization", false);
	obs_data_set_default_int(settings, "dilation_iterations", 0);
	obs_data_set_default_bool(settings, "rescale_image", false);
	obs_data_set_default_int(settings, "rescale_target_size", 35);
	obs_data_set_default_string(settings, "text_sources", "none");
	obs_data_set_default_string(settings, "text_detection_mask_sources", "none");
	obs_data_set_default_string(settings, "char_whitelist",
				    WHITELIST_CHARS_ENGLISH); // default to english characters
	obs_data_set_default_int(settings, "conf_threshold", 50);
	obs_data_set_default_bool(settings, "enable_smoothing", false);
	obs_data_set_default_int(settings, "word_length", 5);
	obs_data_set_default_int(settings, "window_size", 10);
	obs_data_set_default_string(settings, "output_formatting", "{{output}}");
	obs_data_set_default_int(settings, "image_output_option", 0);
	obs_data_set_default_bool(settings, "output_file_append", false);
	obs_data_set_default_bool(settings, "output_flatten", false);
}

void ocr_filter_update(void *data, obs_data_t *settings)
{
	struct filter_data *tf = reinterpret_cast<filter_data *>(data);

	// Update the output text source
	update_text_source_on_settings(tf, settings);
	// Update the output text detection mask image source
	update_image_source_on_settings(tf, settings);

	bool hard_tesseract_init_required = false;

	std::string new_language = obs_data_get_string(settings, "language");
	if (new_language != tf->language) {
		// if the language changed, we need to reinitialize the tesseract model
		hard_tesseract_init_required = true;
	}
	tf->language = new_language;

	tf->pageSegmentationMode = (int)obs_data_get_int(settings, "page_segmentation_mode");
	tf->binarizationMode = (int)obs_data_get_int(settings, "binarization_mode");
	tf->binarizationThreshold = (int)obs_data_get_int(settings, "binarization_threshold");
	tf->binarizationBlockSize = (int)obs_data_get_int(settings, "binarization_block_size");
	tf->previewBinarization = obs_data_get_bool(settings, "preview_binarization");
	tf->dilationIterations = (int)obs_data_get_int(settings, "dilation_iterations");
	tf->rescaleImage = obs_data_get_bool(settings, "rescale_image");
	tf->rescaleTargetSize = (int)obs_data_get_int(settings, "rescale_target_size");
	tf->char_whitelist = obs_data_get_string(settings, "char_whitelist");
	tf->conf_threshold = (int)obs_data_get_int(settings, "conf_threshold");
	tf->enable_smoothing = obs_data_get_bool(settings, "enable_smoothing");
	tf->word_length = obs_data_get_int(settings, "word_length");
	tf->window_size = obs_data_get_int(settings, "window_size");
	tf->update_timer_ms = (uint32_t)obs_data_get_int(settings, "update_timer");
	tf->output_format_template = obs_data_get_string(settings, "output_formatting");
	tf->update_on_change = obs_data_get_bool(settings, "update_on_change");
	tf->update_on_change_threshold =
		(int)obs_data_get_int(settings, "update_on_change_threshold");
	tf->output_image_option = (int)obs_data_get_int(settings, "image_output_option");
	tf->output_file_append = obs_data_get_bool(settings, "output_file_append");
	tf->output_flatten = obs_data_get_bool(settings, "output_flatten");

	// Initialize the Tesseract OCR model
	initialize_tesseract_ocr(tf, hard_tesseract_init_required);
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
	tf->unique_id = obs_source_get_uuid(source);
	tf->texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);
	tf->output_source_name = bstrdup(obs_data_get_string(settings, "text_sources"));
	tf->output_source = nullptr;
	// initialize the mutex
	tf->output_source_mutex = new std::mutex();

	// get the models folder path from the module
	tf->tesseractTraineddataFilepath = obs_module_file("tessdata");

	tf->tesseract_model = nullptr;

	obs_enter_graphics();
	char *error;
	tf->effect = gs_effect_create_from_file(obs_module_file("preview.effect"), &error);
	if (tf->effect == nullptr) {
		obs_log(LOG_ERROR, "Failed to create effect from file: %s", error);
		bfree(error);
	}
	obs_leave_graphics();

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
		if (tf->outputPreviewTexture != nullptr) {
			gs_texture_destroy(tf->outputPreviewTexture);
		}
		if (tf->effect != nullptr) {
			gs_effect_destroy(tf->effect);
		}
		obs_leave_graphics();

		stop_and_join_tesseract_thread(tf);

		cleanup_config_files(tf->unique_id);

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

	// if preview binarization is enabled, render the binarized image
	if (tf->previewBinarization) {
		gs_texture_t *tex = nullptr;
		{
			// lock the outputPreviewBGRALock mutex
			std::lock_guard<std::mutex> lock(tf->outputPreviewBGRALock);
			if (tf->outputPreviewBGRA.empty()) {
				obs_log(LOG_ERROR, "Binarized image is empty");
				obs_source_skip_video_filter(tf->source);
				return;
			}
			if ((uint32_t)tf->outputPreviewBGRA.cols != width ||
			    (uint32_t)tf->outputPreviewBGRA.rows != height) {
				obs_source_skip_video_filter(tf->source);
				return;
			}

			tex = gs_texture_create(width, height, GS_RGBA, 1,
						(const uint8_t **)&tf->outputPreviewBGRA.data, 0);
		}

		gs_eparam_t *imageParam = gs_effect_get_param_by_name(tf->effect, "myimage");
		gs_effect_set_texture(imageParam, tex);

		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

		while (gs_effect_loop(tf->effect, "MyDraw")) {
			gs_draw_sprite(tex, 0, 0, 0);
		}

		gs_blend_state_pop();
		gs_texture_destroy(tex);
	} else {
		obs_source_skip_video_filter(tf->source);
	}
}
