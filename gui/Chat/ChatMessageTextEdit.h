#pragma once

class ChatMessageTextEdit: public QTextEdit
{
public:
	ChatMessageTextEdit(QWidget *parent);

	void showEvent(QShowEvent *event) override;

	bool eventFilter(QObject *o, QEvent *e) override;

private:
	bool _showed = false;
};
