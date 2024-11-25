
#include <opencv2/imgproc.hpp>

#include <numeric>
#include <memory>
#include <exception>
#include <fstream>
#include <new>
#include <mutex>
#include <filesystem>

#include <util/bmem.h>

#include <plugin-support.h>
#include "filter-data.h"
#include "obs-utils.h"
#include "consts.h"
#include "tesseract-ocr-utils.h"
#include "ocr-filter.h"
#include "ocr-filter-callbacks.h"

const char *ocr_filter_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("OCRFilter");
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

	// set the crop region from the properties
	tf->cropRegionRelative.x = (int)obs_data_get_int(settings, "crop_left");
	tf->cropRegionRelative.y = (int)obs_data_get_int(settings, "crop_top");
	tf->cropRegionRelative.width =
		-(int)obs_data_get_int(settings, "crop_right") - tf->cropRegionRelative.x;
	tf->cropRegionRelative.height =
		-(int)obs_data_get_int(settings, "crop_bottom") - tf->cropRegionRelative.y;

	// Initialize the Tesseract OCR model
	initialize_tesseract_ocr(tf, hard_tesseract_init_required);
}

void ocr_filter_activate(void *data)
{
	struct filter_data *tf = reinterpret_cast<filter_data *>(data);
	obs_log(LOG_INFO, "ocr_filter_activate");
	tf->isDisabled = false;
}

void ocr_filter_deactivate(void *data)
{
	struct filter_data *tf = reinterpret_cast<filter_data *>(data);
	obs_log(LOG_INFO, "ocr_filter_deactivate");
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

	signal_handler_t *sh_filter = obs_source_get_signal_handler(tf->source);
	if (sh_filter == nullptr) {
		obs_log(LOG_ERROR, "Failed to get signal handler");
		tf->isDisabled = true;
		return nullptr;
	}

	signal_handler_connect(sh_filter, "enable", enable_callback, tf);

	return tf;
}

void ocr_filter_destroy(void *data)
{
	struct filter_data *tf = reinterpret_cast<filter_data *>(data);

	signal_handler_t *sh_filter = obs_source_get_signal_handler(tf->source);
	signal_handler_disconnect(sh_filter, "enable", enable_callback, tf);

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

			tex = gs_texture_create(tf->outputPreviewBGRA.cols,
						tf->outputPreviewBGRA.rows, GS_RGBA, 1,
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
