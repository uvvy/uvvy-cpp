#pragma once

class Contact
{
public:
	const int VERSION = 1;

public:
	Contact(const QString &id, const QString &userName);

	QString id();
	void setId(const QString &id);

	QString userName();
	void setUserName(const QString &userName);

	static bool loadVersion1(Contact *contact, QDataStream &ds);

private:
	QString _id;
	QString _userName;
};
