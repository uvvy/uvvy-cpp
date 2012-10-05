
#include <QScrollBar>

#include "logarea.h"


LogArea::LogArea(QWidget *parent)
:	QScrollArea(parent)
{
	QScrollBar *scrollbar = verticalScrollBar();

	connect(scrollbar, SIGNAL(valueChanged(int)),
		this, SLOT(scrollValueChanged(int)));
	connect(scrollbar, SIGNAL(rangeChanged(int, int)),
		this, SLOT(scrollRangeChanged(int, int)));

	scrollValue = scrollbar->value();
	scrollMax = scrollbar->maximum();
}

void LogArea::scrollValueChanged(int value)
{
	// Just track the current value
	scrollValue = value;
}

void LogArea::scrollRangeChanged(int, int max)
{
	// If the scroll bar was at the bottom before the new chat entry,
	// keep it at the bottom of the logview afterwards as well.
	if (scrollValue == scrollMax)
		verticalScrollBar()->setSliderPosition(max);

	scrollMax = max;
}

