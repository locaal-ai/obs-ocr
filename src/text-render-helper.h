#ifndef TEXT_RENDER_HELPER_H
#define TEXT_RENDER_HELPER_H

#include "tesseract-ocr-utils.h"

#include <string>
#include <vector>

#include <QImage>

QImage render_boxes_with_qtextdocument(const std::vector<OCRBox> &boxes, uint32_t width,
				       uint32_t height);

#endif // TEXT_RENDER_HELPER_H
