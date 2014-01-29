
#include <QPainter>

#include "meter.h"

Meter::Meter(QWidget *parent)
:	QWidget(parent),
	min(0), max(0), val(0)
{
	setMinimumSize(50, 10);
	setMaximumSize(QWIDGETSIZE_MAX, 10);
}

void Meter::setValue(int val)
{
	this->val = val;
	update();
}

void Meter::setRange(int min, int max)
{
	this->min = min;
	this->max = max;
	update();
}

void Meter::paintEvent(QPaintEvent *)
{
	if (min >= max)
		return;
	float frac = (float)(val-min) / (float)(max-min);

	QPainter p(this);

	int nticks = width() / 10;
	int nfill = (int)(nticks * frac);
	for (int i = 0; i < nticks; i++) {
		p.setBrush(i < nfill ? Qt::cyan : Qt::darkBlue);
		p.setPen(Qt::darkCyan);
		p.drawRect(10*i, 0, 6, height()-1);
	}
}

