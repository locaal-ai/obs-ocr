#ifndef OCR_FILTER_H
#define OCR_FILTER_H

#include <obs-module.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *ocr_filter_getname(void *unused);
void *ocr_filter_create(obs_data_t *settings, obs_source_t *source);
void ocr_filter_destroy(void *data);
void ocr_filter_defaults(obs_data_t *settings);
obs_properties_t *ocr_filter_properties(void *data);
void ocr_filter_update(void *data, obs_data_t *settings);
void ocr_filter_activate(void *data);
void ocr_filter_deactivate(void *data);
void ocr_filter_video_tick(void *data, float seconds);
void ocr_filter_video_render(void *data, gs_effect_t *_effect);

#ifdef __cplusplus
}
#endif

#endif /* OCR_FILTER_H */
