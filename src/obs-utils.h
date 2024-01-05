#ifndef OBS_UTILS_H
#define OBS_UTILS_H

#include "filter-data.h"

bool getRGBAFromStageSurface(filter_data *tf, uint32_t &width,
			     uint32_t &height);

inline bool is_valid_output_source_name(const char *output_source_name)
{
	return output_source_name != nullptr && strcmp(output_source_name, "none") != 0 &&
	       strcmp(output_source_name, "(null)") != 0 && strcmp(output_source_name, "") != 0;
}

void acquire_weak_output_source_ref(struct filter_data *usd);

void setTextCallback(const std::string &str, struct filter_data *usd);

bool add_sources_to_list(void *list_property, obs_source_t *source);

void update_text_source_on_settings(struct filter_data *usd, obs_data_t *settings);


#endif /* OBS_UTILS_H */
