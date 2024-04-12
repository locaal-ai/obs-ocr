#include "obs-utils.h"
#include "plugin-support.h"

#include <obs-module.h>

#include <QImage>
#include <QString>

#include <opencv2/core.hpp>

#include <string>
#include <filesystem>
#include <mutex>
#include <opencv2/imgproc.hpp>
#include <fstream>
#include <regex>

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

void acquire_weak_output_source_ref(struct filter_data *usd, char *output_source_name_for_ref,
				    obs_weak_source_t **output_source)
{
	if (!is_valid_output_source_name(output_source_name_for_ref)) {
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
	obs_source_t *source = obs_get_source_by_name(output_source_name_for_ref);
	if (source) {
		*output_source = obs_source_get_weak_source(source);
		obs_source_release(source);
		if (!*output_source) {
			obs_log(LOG_ERROR, "failed to get weak source for source %s",
				output_source_name_for_ref);
		}
	} else {
		obs_log(LOG_ERROR, "source '%s' not found", output_source_name_for_ref);
	}
}

void setTextCallback(const std::string &str_in, struct filter_data *usd)
{
	if (!usd->output_source_mutex) {
		obs_log(LOG_ERROR, "output_source_mutex is null");
		return;
	}

	std::string str = str_in;
	if (usd->output_flatten) {
		// remove newlines and tabs, replace with spaces
		std::replace(str.begin(), str.end(), '\n', ' ');
		std::replace(str.begin(), str.end(), '\t', ' ');
		std::replace(str.begin(), str.end(), '\r', ' ');

		// remove multiple spaces
		str = std::regex_replace(str, std::regex(" +"), " ");
	}

	// check if save_to_file is selected
	if (strcmp(usd->output_source_name, "!!save_to_file!!") == 0) {
		// save_to_file is selected, write the text to a file
		if (usd->output_file_path.empty()) {
			return;
		}
		// append flag according to tf->output_file_append
		std::ofstream file(usd->output_file_path, usd->output_file_append
								  ? std::ios_base::app
								  : std::ios_base::trunc);
		if (!file.is_open()) {
			obs_log(LOG_ERROR, "failed to open file %s", usd->output_file_path.c_str());
			return;
		}
		file << str;
		file.close();
		return;
	}

	if (!usd->output_source) {
		// attempt to acquire a weak ref to the text source if it's yet available
		acquire_weak_output_source_ref(usd, usd->output_source_name, &(usd->output_source));
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

void setTextDetectionMaskCallback(const cv::Mat &mask_rgba, struct filter_data *usd)
{
	UNUSED_PARAMETER(mask_rgba);
	if (!usd->output_source_mutex) {
		obs_log(LOG_ERROR, "output_source_mutex is null");
		return;
	}

	if (!usd->output_image_source) {
		// attempt to acquire a weak ref to the image source if it's yet available
		acquire_weak_output_source_ref(usd, usd->output_image_source_name,
					       &(usd->output_image_source));
	}

	std::lock_guard<std::mutex> lock(*usd->output_source_mutex);

	obs_weak_source_t *image_source = usd->output_image_source;
	if (!image_source) {
		obs_log(LOG_ERROR, "image_source is null");
		return;
	}
	auto target = obs_weak_source_get_source(image_source);
	if (!target) {
		obs_log(LOG_ERROR, "image_source target is null");
		return;
	}

	// write the mask to a png file
	// get file path in the config folder
	std::string config_folder = obs_module_config_path("");
	std::string filename = config_folder + "/" + usd->unique_id + ".png";
	// write the file
	write_png_file_rgba(filename.c_str(), mask_rgba.data, mask_rgba.cols, mask_rgba.rows);

	// set the image source settings
	auto image_settings = obs_source_get_settings(target);
	obs_data_set_string(image_settings, "file", filename.c_str());
	obs_source_update(target, image_settings);
	obs_data_release(image_settings);
	obs_source_release(target);
}

bool add_sources_to_list(void *list_property, obs_source_t *source,
			 const std::vector<std::string> &source_ids)
{
	// add all text and media sources to the list
	auto source_id = obs_source_get_id(source);
	if (std::find(source_ids.begin(), source_ids.end(), source_id) == source_ids.end()) {
		return true;
	}

	obs_property_t *sources = (obs_property_t *)list_property;
	const char *name = obs_source_get_name(source);
	obs_property_list_add_string(sources, name, name);
	return true;
}

bool add_text_sources_to_list(void *list_property, obs_source_t *source)
{
	return add_sources_to_list(list_property, source,
				   {"text_ft2_source_v2", "text_gdiplus_v2"});
}

bool add_image_sources_to_list(void *list_property, obs_source_t *source)
{
	return add_sources_to_list(list_property, source, {"image_source"});
}

void update_output_source_on_settings(struct filter_data *usd, obs_data_t *settings,
				      const char *setting_prop_name,
				      obs_weak_source_t **output_source, char **output_source_name)
{
	// update the text source
	const char *new_source_name = obs_data_get_string(settings, setting_prop_name);
	obs_weak_source_t *old_weak_source = NULL;

	if (!is_valid_output_source_name(new_source_name)) {
		// new selected text source is not valid, release the old one
		if (*output_source) {
			std::lock_guard<std::mutex> lock(*usd->output_source_mutex);
			old_weak_source = *output_source;
			*output_source = nullptr;
		}
		if (*output_source_name) {
			bfree(*output_source_name);
			*output_source_name = nullptr;
		}
	} else {
		// new selected text source is valid, check if it's different from the old one
		if (*output_source_name == nullptr ||
		    strcmp(new_source_name, *output_source_name) != 0) {
			// new text source is different from the old one, release the old one
			if (*output_source) {
				std::lock_guard<std::mutex> lock(*usd->output_source_mutex);
				old_weak_source = *output_source;
				*output_source = nullptr;
			}
			*output_source_name = bstrdup(new_source_name);
		}
	}

	if (old_weak_source) {
		obs_weak_source_release(old_weak_source);
	}
}

void update_text_source_on_settings(struct filter_data *usd, obs_data_t *settings)
{
	// check if text_sources is pointing to !!save_to_file!!
	const char *text_sources = obs_data_get_string(settings, "text_sources");
	if (strcmp(text_sources, "!!save_to_file!!") != 0) {
		// text_sources is not pointing to !!save_to_file!!, update the selected text source
		update_output_source_on_settings(usd, settings, "text_sources", &usd->output_source,
						 &usd->output_source_name);
		usd->output_file_path = "";
	} else {
		// text_sources is pointing to !!save_to_file!!, release the selected text source
		if (usd->output_source) {
			std::lock_guard<std::mutex> lock(*usd->output_source_mutex);
			obs_weak_source_release(usd->output_source);
			usd->output_source = nullptr;
		}
		usd->output_source_name = bstrdup(text_sources);
		usd->output_file_path = obs_data_get_string(settings, "output_file_path");
	}
}

void update_image_source_on_settings(struct filter_data *usd, obs_data_t *settings)
{
	update_output_source_on_settings(usd, settings, "text_detection_mask_sources",
					 &usd->output_image_source, &usd->output_image_source_name);
}

void check_plugin_config_folder_exists()
{
	std::string config_folder = obs_module_config_path("");
	if (!std::filesystem::exists(config_folder)) {
		obs_log(LOG_INFO, "Creating plugin config folder: %s", config_folder.c_str());
		std::filesystem::create_directories(config_folder);
	}
}

void write_png_file_8uc1(const char *filename, const unsigned char *image8uc1, int width,
			 int height)
{
	QImage image(image8uc1, width, height, QImage::Format_Grayscale8);
	QString qfilename(filename);
	image.save(qfilename);
}

void write_png_file_rgba(const char *filename, const unsigned char *imageRGBA, int width,
			 int height)
{
	QImage image(imageRGBA, width, height, QImage::Format_RGBA8888);
	QString qfilename(filename);
	image.save(qfilename);
}
