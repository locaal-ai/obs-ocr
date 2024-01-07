#include "tesseract-ocr-utils.h"
#include "plugin-support.h"
#include "obs-utils.h"

#include <obs-module.h>

#include <opencv2/core/mat.hpp>

#include <tesseract/baseapi.h>

#include <string>
#include <fstream>
#include <deque>
#include <stdexcept>
#include <algorithm>

void initialize_tesseract_ocr(filter_data *tf)
{
	// Load model
	obs_log(LOG_INFO, "Loading tesseract model from: %s", tf->tesseractTraineddataFilepath);
	try {
		if (tf->tesseract_model != nullptr) {
			tf->tesseract_model->End();
			delete tf->tesseract_model;
			tf->tesseract_model = nullptr;
		}

		tf->tesseract_model = new tesseract::TessBaseAPI();
		// set tesseract page segmentation mode to single word
		tf->tesseract_model->Init(tf->tesseractTraineddataFilepath, tf->language.c_str());
		tf->tesseract_model->SetPageSegMode(
			static_cast<tesseract::PageSegMode>(tf->pageSegmentationMode));

		// apply char whitlist
		tf->tesseract_model->SetVariable("tessedit_char_whitelist",
						 tf->char_whitelist.c_str());

		// if the user patterns are not empty, apply them
		if (!tf->user_patterns.empty()) {
			check_plugin_config_folder_exists();
			// save the user patterns to a file in the module's config folder
			std::string user_patterns_filepath =
				obs_module_config_path("user-patterns.txt");
			obs_log(LOG_INFO, "Saving user patterns to: %s",
				user_patterns_filepath.c_str());
			std::ofstream user_patterns_file(user_patterns_filepath);
			user_patterns_file << tf->user_patterns;
			user_patterns_file.close();
			// apply the user patterns file
			tf->tesseract_model->SetVariable("user_patterns_file",
							 user_patterns_filepath.c_str());
		}

		if (tf->enable_smoothing) {
			tf->smoothing_filter = std::make_unique<CharacterBasedSmoothingFilter>(
				tf->word_length, tf->window_size);
		}
	} catch (std::exception &e) {
		obs_log(LOG_ERROR, "Failed to load tesseract model: %s", e.what());
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
	tf->tesseract_model->SetImage(imageBGRA.data, imageBGRA.cols, imageBGRA.rows, 4,
				      (int)imageBGRA.step);
	std::string recognitionResult = std::string(tf->tesseract_model->GetUTF8Text());
	// get the confidence of the recognition result
	const int confidence = tf->tesseract_model->MeanTextConf();

	if (confidence < tf->conf_threshold) {
		return "";
	}

	// strip whitespace from the beginning and end of the string
	recognitionResult = strip(recognitionResult);

	if (tf->enable_smoothing) {
		recognitionResult = tf->smoothing_filter->add_reading(recognitionResult);
	}

	obs_log(LOG_DEBUG, "OCR result: %s", recognitionResult.c_str());

	return recognitionResult;
}

CharacterBasedSmoothingFilter::CharacterBasedSmoothingFilter(size_t word_length_,
							     size_t window_size_)
	: word_length(word_length_),
	  window_size(window_size_),
	  readings(word_length_, std::deque<char>(window_size_))
{
}

std::string CharacterBasedSmoothingFilter::add_reading(const std::string &inWord)
{
	std::string word = inWord;
	if (word.length() != word_length) {
		// trim the word if it's longer than the expected length
		if (word.length() > this->word_length)
			word = word.substr(0, this->word_length);
		// pad the word if it's shorter than the expected length
		if (word.length() < this->word_length)
			word = word + std::string(this->word_length - word.length(), ' ');
	}

	std::string smoothed_word;
	for (size_t i = 0; i < word_length; i++) {
		readings[i].push_back(word[i]);
		if (readings[i].size() > window_size) {
			readings[i].pop_front();
		}
		std::string window(readings[i].begin(), readings[i].end());
		// find the most common character in the window
		char most_common_char =
			*std::max_element(window.begin(), window.end(), [window](char a, char b) {
				return std::count(window.begin(), window.end(), a) <
				       std::count(window.begin(), window.end(), b);
			});
		smoothed_word += most_common_char;
	}

	return smoothed_word;
}
