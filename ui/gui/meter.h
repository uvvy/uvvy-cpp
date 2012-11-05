#ifndef METER_H
#define METER_H

#include <QWidget>

class Meter : public QWidget
{
	Q_OBJECT

private:
	int min, max, val;

public:
	Meter(QWidget *parent = NULL);

	inline int minimum() { return min; }
	inline int maximum() { return max; }
	inline int value() { return val; }

public slots:
	void setRange(int min, int max);
	void setValue(int val);

protected:
	virtual void paintEvent(QPaintEvent *event);
};

#endif	// METER_H
