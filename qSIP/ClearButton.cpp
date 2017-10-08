#include "ClearButton.h"

#include <QPainter>

ClearButton::ClearButton(QWidget *parent)
	: QToolButton(parent)
{
	pixmap = QPixmap(":/image/clear.png");
	int w = pixmap.width();
	int h = pixmap.height();
	if (w > 0 && h > 0) {
		setFixedSize(w + 9, h + 8);
	}
}

void ClearButton::paintEvent(QPaintEvent * /*event*/)
{
	QPainter pr(this);
	pr.setOpacity(underMouse() ? 1.0 : 0.5);
	int w = pixmap.width();
	int h = pixmap.height();
	int x = (width() - w) / 2;
	int y = (height() - h) / 2;
	int delta = isDown() ? 1 : 0;
	pr.drawPixmap(x + delta, y + delta, w, h, pixmap);
}
