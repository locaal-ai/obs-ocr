#ifndef TESSERACT_OCR_UTILS_H
#define TESSERACT_OCR_UTILS_H

#include "filter-data.h"

#include <deque>
#include <string>

void cleanup_config_files(const std::string &unique_id);
void initialize_tesseract_ocr(filter_data *tf, bool hard_tesseract_init_required = false);
std::string run_tesseract_ocr(filter_data *tf, const cv::Mat &imageBGRA);
std::string strip(const std::string &str);
void stop_and_join_tesseract_thread(struct filter_data *tf);
void tesseract_thread(void *data);

class CharacterBasedSmoothingFilter {
public:
	CharacterBasedSmoothingFilter(size_t word_length, size_t window_size = 10);

	std::string add_reading(const std::string &word);

private:
	size_t word_length;
	size_t window_size;
	std::vector<std::deque<char>> readings;
};

#endif /* TESSERACT_OCR_UTILS_H */
