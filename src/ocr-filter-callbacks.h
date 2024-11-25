#pragma once

#include <callback/calldata.h>

void media_play_callback(void *data_, calldata_t *cd);
void media_started_callback(void *data_, calldata_t *cd);
void media_pause_callback(void *data_, calldata_t *cd);
void media_restart_callback(void *data_, calldata_t *cd);
void media_stopped_callback(void *data_, calldata_t *cd);
void enable_callback(void *data_, calldata_t *cd);
