#pragma once

class ChatItem
{
public:
	ChatItem(const QString &nickName, const QString &text, const QTime &time, const QBrush &nickColor)
		: _nickName(nickName), _text(text), _time(time), _nickColor(nickColor)
	{

	}

	void setNickName(const QString &nickName)
	{
		_nickName = nickName;
	}

	const QString &nickName() const
	{
		return _nickName;
	}

	void setText(const QString &text)
	{
		_text = text;
	}

	const QString &text() const
	{
		return _text;
	}

	void setTime(const QTime &time)
	{
		_time = time;
	}

	const QTime &time()
	{
		return _time;
	}

	void setNickColor(const QBrush &nickColor)
	{
		_nickColor = nickColor;
	}

	const QBrush nickColor() const
	{
		return _nickColor;
	}

private:
	QString _nickName;
	QString _text;
	QTime _time;
	QBrush _nickColor;
};
