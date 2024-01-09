#include "ocr-filter.h"

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
	.video_render = ocr_filter_video_render,
};
