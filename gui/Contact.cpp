#include "Contact.h"

Contact::Contact(const QString &id, const QString &userName)
		: _id(id)
		, _userName(userName)
{

}

QString Contact::id()
{
	return _id;
}

void Contact::setId(const QString &id)
{
	_id = id;
}

QString Contact::userName()
{
	return _userName;
}

void Contact::setUserName(const QString &userName)
{
	_userName = userName;
}

bool Contact::loadVersion1(Contact *contact, QDataStream &ds)
{
	QString id;
	QString userName;

	ds >> id >> userName;

	contact->setId(id);
	contact->setUserName(userName);

	return true;
}

/*
bool Contact::loadVersion2(Contact *contact, QDataStream &ds)
{
	if(!loadVersion1(contact, ds))
	{
		return false;
	}

	QString param1;
	QString param2;
	QString param3;

	ds >> param1 >> param2 >> param3;

	contact->setParam1(param1);
	contact->setParam2(param2);
	contact->setParam3(param3);

	return true;
}
*/
