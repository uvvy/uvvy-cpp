#pragma once

#include <QSet>
#include <QList>
#include <QString>
#include <QByteArray>
#include <QDialog>
#include <QTableWidget>

#include "regcli.h"

class QLineEdit;
class QTableView;
class QTableWidgetItem;
namespace SST {
	class RegInfo;
	class Endpoint;
}

class SearchDialog : public QDialog
{
	Q_OBJECT

private:
	typedef SST::Endpoint Endpoint;
	typedef SST::RegInfo RegInfo;

	QLineEdit *textline;
	QTableWidget *results;

	QString searchtext;
	QHash<QByteArray, int> reqids;
	QList<QByteArray> resultids;

public:
	SearchDialog(QWidget *parent);

	void present();


private:
	void closeEvent(QCloseEvent *event);
	QTableWidgetItem *item(const QString &text);

private slots:
	void startSearch();
	void searchDone(const QString &text, const QList<QByteArray> ids,
			bool complete);
	void lookupDone(const QByteArray &id, const Endpoint &loc,
			const RegInfo &info);
	void addPeer();
};
