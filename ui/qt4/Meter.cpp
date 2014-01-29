#include <QPainter>
#include "Meter.h"

Meter::Meter(QWidget *parent)
    : QWidget(parent)
    , min_(0)
    , max_(0)
    , value_(0)
{
    setMinimumSize(50, 10);
    setMaximumSize(QWIDGETSIZE_MAX, 10);
}

void Meter::setValue(int val)
{
    value_ = val;
    update();
}

void Meter::setRange(int min, int max)
{
    min_ = min;
    max_ = max;
    update();
}

void Meter::paintEvent(QPaintEvent *)
{
    if (min_ >= max_) {
        return;
    }
    float frac = (float)(value_ - min_) / (float)(max_ - min_);

    QPainter p(this);

    int nticks = width() / 10;
    int nfill = (int)(nticks * frac);
    for (int i = 0; i < nticks; i++)
    {
        p.setBrush(i < nfill ? Qt::cyan : Qt::darkBlue);
        p.setPen(Qt::darkCyan);
        p.drawRect(10*i, 0, 6, height()-1);
    }
}

