#include "text-render-helper.h"

#include <QPainter>
#include <QPixmap>

/**
  * Render text to a buffer using QTextDocument
  * @param text Text to render
  * @param width Output width
  * @param height Output height
  * @param data Output buffer, user must free
	* @param css_props CSS properties to apply to the text
  */
QImage render_boxes_with_qtextdocument(const std::vector<OCRBox> &boxes, uint32_t width,
				       uint32_t height, bool add_background)
{
	QPixmap pixmap(width, height);
	pixmap.fill(Qt::transparent);
	QPainter painter;
	painter.begin(&pixmap);
	painter.setCompositionMode(QPainter::CompositionMode_Source);

	// draw individual boxes on the pixmap
	for (const OCRBox &box : boxes) {
		if (add_background) {
			painter.setBrush(Qt::white);
			painter.fillRect(box.box.x, box.box.y, box.box.width, box.box.height,
					 Qt::white);
		}
		painter.setPen(Qt::blue);
		// set the character size according to the box height
		QFont font = painter.font();
		font.setPixelSize(box.box.height);
		painter.setFont(font);
		painter.drawText(box.box.x, box.box.y + box.box.height,
				 QString::fromStdString(box.text));
	}

	painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
	painter.end();

	return pixmap.toImage();
}
