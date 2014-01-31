#pragma once

#include <QWidget>

class Meter : public QWidget
{
    Q_OBJECT

private:
    int min_, max_, value_;

public:
    Meter(QWidget *parent = NULL);

    inline int minimum() { return min_; }
    inline int maximum() { return max_; }
    inline int value() { return value_; }

public slots:
    void setRange(int min, int max);
    void setValue(int val);

protected:
    virtual void paintEvent(QPaintEvent *event);
};

