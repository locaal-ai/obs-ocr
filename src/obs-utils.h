#ifndef OBS_UTILS_H
#define OBS_UTILS_H

#include "filter-data.h"

bool getRGBAFromStageSurface(filter_data *tf, uint32_t &width, uint32_t &height);

inline bool is_valid_output_source_name(const char *output_source_name)
{
	return output_source_name != nullptr && strcmp(output_source_name, "none") != 0 &&
	       strcmp(output_source_name, "(null)") != 0 && strcmp(output_source_name, "") != 0;
}

void acquire_weak_output_source_ref(struct filter_data *usd);

void setTextCallback(const std::string &str, struct filter_data *usd);
void setTextDetectionMaskCallback(const cv::Mat &mask, struct filter_data *usd);

bool add_text_sources_to_list(void *list_property, obs_source_t *source);

bool add_image_sources_to_list(void *list_property, obs_source_t *source);

void update_text_source_on_settings(struct filter_data *usd, obs_data_t *settings);
void update_image_source_on_settings(struct filter_data *usd, obs_data_t *settings);

void check_plugin_config_folder_exists();

void write_png_file_8uc1(const char *filename, const unsigned char *image8uc3, int width,
			 int height);
void write_png_file_rgba(const char *filename, const unsigned char *imageRGBA, int width,
			 int height);

#endif /* OBS_UTILS_H */
