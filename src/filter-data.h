#ifndef FILTERDATA_H
#define FILTERDATA_H

#include <obs-module.h>

#include <opencv2/core/mat.hpp>

#include <tesseract/baseapi.h>

#include <mutex>
#include <string>
#include <thread>
#include <condition_variable>

class CharacterBasedSmoothingFilter;

/**
  * @brief The filter_data struct
  *
*/
struct filter_data {
	std::string modelSelection;

	obs_source_t *source;
	std::string unique_id;
	gs_texrender_t *texrender;
	gs_stagesurf_t *stagesurface;

	cv::Mat inputBGRA;
	cv::Mat lastInputBGRA;
	tesseract::TessBaseAPI *tesseract_model;
	std::string language;
	int pageSegmentationMode;
	std::string char_whitelist;
	std::string user_patterns;
	int conf_threshold;
	bool enable_smoothing;
	std::unique_ptr<CharacterBasedSmoothingFilter> smoothing_filter;
	size_t word_length;
	size_t window_size;
	uint32_t update_timer_ms;
	std::string output_format_template;
	bool update_on_change;
	int update_on_change_threshold;

	bool isDisabled;

	std::mutex inputBGRALock;
	std::mutex outputLock;
	std::mutex tesseract_mutex;
	bool tesseract_thread_run;
	std::condition_variable tesseract_thread_cv;
	std::thread tesseract_thread;

	// Text source to output the text to
	obs_weak_source_t *output_source = nullptr;
	char *output_source_name = nullptr;
	// Image source to output the detection mask to
	obs_weak_source_t *output_image_source = nullptr;
	char *output_image_source_name = nullptr;
	std::mutex *output_source_mutex = nullptr;

	char *tesseractTraineddataFilepath = nullptr;
};

#endif /* FILTERDATA_H */
