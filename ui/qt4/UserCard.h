#pragma once

#include <QWidget>
#include <QItemDelegate>
#include "ui_UserCard.h"

class UserCard : public QWidget, private Ui::UserCard
{
    Q_OBJECT

public:
    UserCard(QWidget* parent = nullptr) : QWidget(parent) {}
};

class UserCardDelegate : public QItemDelegate
{
    UserCard* widget;
};
