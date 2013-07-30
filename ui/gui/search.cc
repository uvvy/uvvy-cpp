#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include <QtDebug>

#include "search.h"
#include "regcli.h"
#include "main.h"

using namespace SST;


SearchDialog::SearchDialog(QWidget *parent)
:	QDialog(parent)
{
	setWindowTitle(tr("Search for Friends"));

	foreach (RegClient *cli, regclients) {
		connect(cli, SIGNAL(searchDone(const QString &,
						const QList<SST::PeerId>,
						bool)),
			this, SLOT(searchDone(const QString &,
						const QList<SST::PeerId>,
						bool)));
		connect(cli, SIGNAL(lookupDone(const SST::PeerId &,
						const Endpoint &,
						const RegInfo &)),
			this, SLOT(lookupDone(const SST::PeerId &,
						const Endpoint &,
						const RegInfo &)));
	}

	textline = new QLineEdit(this);
	connect(textline, SIGNAL(returnPressed()), this, SLOT(startSearch()));

	QPushButton *searchbutton = new QPushButton(tr("Search"), this);
	connect(searchbutton, SIGNAL(clicked()), this, SLOT(startSearch()));

	QHBoxLayout *searchlayout = new QHBoxLayout();
	searchlayout->addWidget(textline);
	searchlayout->addWidget(searchbutton);

	results = new QTableWidget(this);
	results->setSelectionBehavior(QTableWidget::SelectRows);
	results->setColumnCount(6);
	results->setHorizontalHeaderItem(0,
			new QTableWidgetItem(tr("User Name")));
	results->setHorizontalHeaderItem(1,
			new QTableWidgetItem(tr("Host Name")));
	results->setHorizontalHeaderItem(2,
			new QTableWidgetItem(tr("City")));
	results->setHorizontalHeaderItem(3,
			new QTableWidgetItem(tr("Region")));
	results->setHorizontalHeaderItem(4,
			new QTableWidgetItem(tr("Country")));
	results->setHorizontalHeaderItem(5,
			new QTableWidgetItem(tr("Network Address")));
	results->verticalHeader()->hide();
	connect(results, SIGNAL(doubleClicked(const QModelIndex &)),
		this, SLOT(addPeer()));

	QPushButton *addbutton = new QPushButton(tr("Add as Friend"), this);
	connect(addbutton, SIGNAL(clicked()), this, SLOT(addPeer()));

	QPushButton *closebutton = new QPushButton(tr("Close"), this);
	connect(closebutton, SIGNAL(clicked()), this, SLOT(close()));

	QHBoxLayout *buttonlayout = new QHBoxLayout();
	buttonlayout->addWidget(addbutton);
	buttonlayout->addStretch();
	buttonlayout->addWidget(closebutton);

	QVBoxLayout *layout = new QVBoxLayout();
	layout->addLayout(searchlayout);
	layout->addWidget(results);
	layout->addLayout(buttonlayout);
	setLayout(layout);

	// Retrieve the window settings
	settings->beginGroup("SearchWindow");
	move(settings->value("pos", QPoint(100, 100)).toPoint());
	resize(settings->value("size", QSize(500, 300)).toSize());
	settings->endGroup();
}

void SearchDialog::present()
{
	show();
	raise();
	activateWindow();

	startSearch();
}

void SearchDialog::closeEvent(QCloseEvent *event)
{
	// Save the window settings
	settings->beginGroup("SearchWindow");
	settings->setValue("pos", pos());
	settings->setValue("size", size());
	settings->endGroup();

	// Otherwise handle the close event as usual
	QDialog::closeEvent(event);
}

void SearchDialog::startSearch()
{
	searchtext = textline->text();
	reqids.clear();
	resultids.clear();
	results->setRowCount(0);

	bool found = false;
	foreach (RegClient *cli, regclients) {
		if (!cli->registered())
			continue;
		cli->search(searchtext);
		found = true;
	}
	if (!found)
		QMessageBox::warning(this, tr("Cannot Search"),
				tr("You are not currently connected to any registration servers to search."),
				QMessageBox::Ok, QMessageBox::NoButton);
}

void SearchDialog::searchDone(const QString &text, const QList<SST::PeerId> ids, bool)
{
	if (text != searchtext) {
		qDebug("Got results for wrong search text (from old search?)");
		return;
	}
	foreach (const SST::PeerId &id, ids) {
		if (reqids.contains(id))
			continue;

		// Found a new result ID - request info about it.
		qDebug() << "Looking up ID" << id;
		reqids.insert(id, -1);
		foreach (RegClient *cli, regclients) {
			if (!cli->registered())
				continue;
			cli->lookup(id);
		}
	}
}

void SearchDialog::lookupDone(const SST::PeerId &id, const Endpoint &loc, const RegInfo &info)
{
	if (!reqids.contains(id)) {
		qDebug("Got lookup info for wrong ID (from old search?)");
		return;
	}
	qDebug() << "Got RegInfo for ID" << id;

	// Find or create the table row for this ID.
	int &idrow = reqids[id];
	if (idrow >= 0) {
		// Got information about this ID from multiple sources -
		// just assume it's the same for each one.
		qDebug("Got information about ID from multiple sources");
		Q_ASSERT(resultids[idrow] == id);
		return;
	}
	idrow = resultids.size();
	resultids.append(id);

	// Insert the appropriate information in the table
	results->setRowCount(idrow+1);
	results->setItem(idrow, 0, item(info.ownerName()));
	results->setItem(idrow, 1, item(info.hostName()));
	results->setItem(idrow, 2, item(info.city()));
	results->setItem(idrow, 3, item(info.region()));
	results->setItem(idrow, 4, item(info.country()));
	results->setItem(idrow, 5, item(loc.toString()));

	foreach (const Endpoint e, info.endpoints())
	{
		qDebug() << "** Endpoint" << e;
	}
}

QTableWidgetItem *SearchDialog::item(const QString &text)
{
	QTableWidgetItem *i = new QTableWidgetItem(text);
	i->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	return i;
}

void SearchDialog::addPeer()
{
	int row = results->selectionModel()->currentIndex().row();
	if (row < 0 || row >= resultids.size())
		return;

	close();

	// Find an appropriate default name for this friend
	QString name = results->item(row, 0)->text();	// Owner name
	if (name.isEmpty())
		name = results->item(row, 1)->text();	// Host name

	// Add, if it isn't there already
	mainwin->addPeer(resultids[row].getId(), name, true);
}

