#include "tesseract-ocr-utils.h"
#include "plugin-support.h"

#include <obs-module.h>

void initialize_tesseract_ocr(filter_data *tf)
{
	// Load model
	obs_log(LOG_INFO, "Loading tesseract model from: %s",
		tf->tesseractTraineddataFilepath);
	try {
		if (tf->tesseract_model != nullptr) {
			tf->tesseract_model->End();
			delete tf->tesseract_model;
			tf->tesseract_model = nullptr;
		}

		tf->tesseract_model = new tesseract::TessBaseAPI();
		// set tesseract page segmentation mode to single word
		tf->tesseract_model->Init(tf->tesseractTraineddataFilepath,
					  tf->language);
		tf->tesseract_model->SetPageSegMode(
			tesseract::PageSegMode::PSM_SINGLE_WORD);
	} catch (std::exception &e) {
		obs_log(LOG_ERROR, "Failed to load tesseract model: %s",
			e.what());
		return;
	}
}

std::string strip(const std::string &str)
{
	size_t start = str.find_first_not_of(" \t\n\r");
	size_t end = str.find_last_not_of(" \t\n\r");

	if (start == std::string::npos || end == std::string::npos)
		return "";

	return str.substr(start, end - start + 1);
}

std::string run_tesseract_ocr(filter_data *tf, const cv::Mat &imageBGRA)
{
	// run the tesseract model
	tf->tesseract_model->SetImage(imageBGRA.data, imageBGRA.cols,
				      imageBGRA.rows, 4, (int)imageBGRA.step);
	std::string recognitionResult =
		std::string(tf->tesseract_model->GetUTF8Text());

	// strip whitespace from the beginning and end of the string
	recognitionResult = strip(recognitionResult);

	obs_log(LOG_DEBUG, "OCR result: %s", recognitionResult.c_str());

	return recognitionResult;
}
