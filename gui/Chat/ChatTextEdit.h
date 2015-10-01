#pragma once

#include <QPlainTextEdit>

class ChatTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    ChatTextEdit(QWidget* parentWidget);
    ~ChatTextEdit();

    void keyPressEvent(QKeyEvent* event) override;

private:
signals:
    void returnPressed();
};
