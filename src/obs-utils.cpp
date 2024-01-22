#include "obs-utils.h"
#include "plugin-support.h"

#include <obs-module.h>

#include <opencv2/core.hpp>

#include <string>
#include <filesystem>
#include <mutex>

#include <iostream>
#include <fstream>
using namespace std; // needed for ofstream.

/**
  * @brief Get RGBA from the stage surface
  *
  * @param tf  The filter data
  * @param width  The width of the stage surface (output)
  * @param height  The height of the stage surface (output)
  * @return true  if successful
  * @return false if unsuccessful
*/
bool getRGBAFromStageSurface(filter_data *tf, uint32_t &width, uint32_t &height)
{

	if (!obs_source_enabled(tf->source)) {
		return false;
	}

	obs_source_t *target = obs_filter_get_target(tf->source);
	if (!target) {
		return false;
	}
	width = obs_source_get_base_width(target);
	height = obs_source_get_base_height(target);
	if (width == 0 || height == 0) {
		return false;
	}
	gs_texrender_reset(tf->texrender);
	if (!gs_texrender_begin(tf->texrender, width, height)) {
		return false;
	}
	struct vec4 background;
	vec4_zero(&background);
	gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);
	gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f,
		 100.0f);
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	obs_source_video_render(target);
	gs_blend_state_pop();
	gs_texrender_end(tf->texrender);

	if (tf->stagesurface) {
		uint32_t stagesurf_width = gs_stagesurface_get_width(tf->stagesurface);
		uint32_t stagesurf_height = gs_stagesurface_get_height(tf->stagesurface);
		if (stagesurf_width != width || stagesurf_height != height) {
			gs_stagesurface_destroy(tf->stagesurface);
			tf->stagesurface = nullptr;
		}
	}
	if (!tf->stagesurface) {
		tf->stagesurface = gs_stagesurface_create(width, height, GS_BGRA);
	}
	gs_stage_texture(tf->stagesurface, gs_texrender_get_texture(tf->texrender));
	uint8_t *video_data;
	uint32_t linesize;
	if (!gs_stagesurface_map(tf->stagesurface, &video_data, &linesize)) {
		return false;
	}
	{
		std::lock_guard<std::mutex> lock(tf->inputBGRALock);
		tf->inputBGRA = cv::Mat(height, width, CV_8UC4, video_data, linesize);
	}
	gs_stagesurface_unmap(tf->stagesurface);
	return true;
}

/*            OUTPUT TEXT SOURCE UTIL             */

void acquire_weak_output_source_ref(struct filter_data *usd)
{
	if (!is_valid_output_source_name(usd->output_source_name)) {
		obs_log(LOG_ERROR, "output_source_name is invalid");
		// text source is not selected
		return;
	}

	if (!usd->output_source_mutex) {
		obs_log(LOG_ERROR, "output_source_mutex is null");
		return;
	}

	std::lock_guard<std::mutex> lock(*usd->output_source_mutex);

	// acquire a weak ref to the new text source
	obs_source_t *source = obs_get_source_by_name(usd->output_source_name);
	if (source) {
		usd->output_source = obs_source_get_weak_source(source);
		obs_source_release(source);
		if (!usd->output_source) {
			obs_log(LOG_ERROR, "failed to get weak source for text source %s",
				usd->output_source_name);
		}
	} else {
		obs_log(LOG_ERROR, "text source '%s' not found", usd->output_source_name);
	}
}

void setTextCallback(const std::string &str, struct filter_data *usd)
{
	if (!usd->output_source_mutex) {
		obs_log(LOG_ERROR, "output_source_mutex is null");
		return;
	}

	if (!usd->output_source) {
		// attempt to acquire a weak ref to the text source if it's yet available
		acquire_weak_output_source_ref(usd);
	}

	if (usd->enable_log_to_file) {
		// Create and open a text file
		ofstream ocr_output_file(usd->log_to_file_file_name);
		ocr_output_file << str.c_str();
		ocr_output_file.close();
	}

	std::lock_guard<std::mutex> lock(*usd->output_source_mutex);

	obs_weak_source_t *text_source = usd->output_source;
	if (!text_source) {
		obs_log(LOG_ERROR, "text_source is null");
		return;
	}
	auto target = obs_weak_source_get_source(text_source);
	if (!target) {
		obs_log(LOG_ERROR, "text_source target is null");
		return;
	}
	auto text_settings = obs_source_get_settings(target);
	obs_data_set_string(text_settings, "text", str.c_str());
	obs_source_update(target, text_settings);
	obs_data_release(text_settings);
	obs_source_release(target);
};

bool add_sources_to_list(void *list_property, obs_source_t *source)
{
	// add all text and media sources to the list
	auto source_id = obs_source_get_id(source);
	if (strcmp(source_id, "text_ft2_source_v2") != 0 &&
	    strcmp(source_id, "text_gdiplus_v2") != 0) {
		return true;
	}

	obs_property_t *sources = (obs_property_t *)list_property;
	const char *name = obs_source_get_name(source);
	std::string name_with_prefix = std::string("(Text) ").append(name);
	obs_property_list_add_string(sources, name_with_prefix.c_str(), name);
	return true;
}

void update_text_source_on_settings(struct filter_data *usd, obs_data_t *settings)
{
	// update the text source
	const char *new_text_source_name = obs_data_get_string(settings, "text_sources");
	obs_weak_source_t *old_weak_text_source = NULL;

	if (!is_valid_output_source_name(new_text_source_name)) {
		// new selected text source is not valid, release the old one
		if (usd->output_source) {
			std::lock_guard<std::mutex> lock(*usd->output_source_mutex);
			old_weak_text_source = usd->output_source;
			usd->output_source = nullptr;
		}
		if (usd->output_source_name) {
			bfree(usd->output_source_name);
			usd->output_source_name = nullptr;
		}
	} else {
		// new selected text source is valid, check if it's different from the old one
		if (usd->output_source_name == nullptr ||
		    strcmp(new_text_source_name, usd->output_source_name) != 0) {
			// new text source is different from the old one, release the old one
			if (usd->output_source) {
				std::lock_guard<std::mutex> lock(*usd->output_source_mutex);
				old_weak_text_source = usd->output_source;
				usd->output_source = nullptr;
			}
			usd->output_source_name = bstrdup(new_text_source_name);
		}
	}

	if (old_weak_text_source) {
		obs_weak_source_release(old_weak_text_source);
	}
}

void check_plugin_config_folder_exists()
{
	std::string config_folder = obs_module_config_path("");
	if (!std::filesystem::exists(config_folder)) {
		obs_log(LOG_INFO, "Creating plugin config folder: %s", config_folder.c_str());
		std::filesystem::create_directories(config_folder);
	}
}
