
#include "filter-data.h"
#include "plugin-support.h"

#include <obs.h>

void media_play_callback(void *data_, calldata_t *cd)
{
	UNUSED_PARAMETER(cd);
	filter_data *gf_ = static_cast<struct filter_data *>(data_);
	obs_log(LOG_INFO, "media_play");
	gf_->isDisabled = false;
}

void media_started_callback(void *data_, calldata_t *cd)
{
	UNUSED_PARAMETER(cd);
	filter_data *gf_ = static_cast<struct filter_data *>(data_);
	obs_log(LOG_INFO, "media_started");
	gf_->isDisabled = false;
}

void media_pause_callback(void *data_, calldata_t *cd)
{
	UNUSED_PARAMETER(cd);
	filter_data *gf_ = static_cast<struct filter_data *>(data_);
	obs_log(LOG_INFO, "media_pause");
	gf_->isDisabled = true;
}

void media_restart_callback(void *data_, calldata_t *cd)
{
	UNUSED_PARAMETER(cd);
	filter_data *gf_ = static_cast<struct filter_data *>(data_);
	obs_log(LOG_INFO, "media_restart");
	gf_->isDisabled = false;
}

void media_stopped_callback(void *data_, calldata_t *cd)
{
	UNUSED_PARAMETER(cd);
	filter_data *gf_ = static_cast<struct filter_data *>(data_);
	obs_log(LOG_INFO, "media_stopped");
	gf_->isDisabled = true;
}

void enable_callback(void *data_, calldata_t *cd)
{
	filter_data *gf_ = static_cast<struct filter_data *>(data_);
	bool enable = calldata_bool(cd, "enabled");
	if (enable) {
		obs_log(LOG_INFO, "enable_callback: enable");
		gf_->isDisabled = false;
	} else {
		obs_log(LOG_INFO, "enable_callback: disable");
		gf_->isDisabled = true;
	}
}
