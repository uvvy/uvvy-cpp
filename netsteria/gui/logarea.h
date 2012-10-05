// 
// LogArea: a simple variant of a QScrollArea for log views,
// which by default keeps the vertical scrollbar pegged to the bottom
// of the internal area as the internal widget's virtual size changes,
// but still allows the user to scroll back in the log when desired.
//
#ifndef LOGAREA_H
#define LOGAREA_H

#include <QScrollArea>

class LogArea : public QScrollArea
{
	Q_OBJECT

private:
	// These variables track the vertical scrollbar's
	// current and maximum value in order to keep it pegged.
	int scrollValue, scrollMax;

public:
	LogArea(QWidget *parent = NULL);

private slots:
	void scrollValueChanged(int value);
	void scrollRangeChanged(int min, int max);
};

#endif	// LOGAREA_H
