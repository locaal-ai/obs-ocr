#ifndef FILTERDATA_H
#define FILTERDATA_H

#include <obs-module.h>

#include <opencv2/core/mat.hpp>

#include <tesseract/baseapi.h>

#include <mutex>
#include <string>

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
	const char *language;
	int pageSegmentationMode;

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
