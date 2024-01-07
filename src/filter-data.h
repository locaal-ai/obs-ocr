#ifndef FILTERDATA_H
#define FILTERDATA_H

#include <obs-module.h>

#include <opencv2/core/mat.hpp>

#include <tesseract/baseapi.h>

#include <mutex>
#include <string>

class CharacterBasedSmoothingFilter;

/**
  * @brief The filter_data struct
  *
*/
struct filter_data {
	std::string modelSelection;

	obs_source_t *source;
	gs_texrender_t *texrender;
	gs_stagesurf_t *stagesurface;

	cv::Mat inputBGRA;
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

	bool isDisabled;

	std::mutex inputBGRALock;
	std::mutex outputLock;

	// Text source to output the text to
	obs_weak_source_t *output_source = nullptr;
	char *output_source_name = nullptr;
	std::mutex *output_source_mutex = nullptr;

	char *tesseractTraineddataFilepath = nullptr;
};

#endif /* FILTERDATA_H */
