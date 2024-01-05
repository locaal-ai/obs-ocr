#ifndef TESSERACT_OCR_UTILS_H
#define TESSERACT_OCR_UTILS_H

#include "filter-data.h"

void initialize_tesseract_ocr(filter_data *tf);
std::string run_tesseract_ocr(filter_data *tf, const cv::Mat &imageBGRA);

#endif /* TESSERACT_OCR_UTILS_H */
