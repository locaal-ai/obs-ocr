
#include "filter-data.h"
#include "plugin-support.h"

#include <obs.h>

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
