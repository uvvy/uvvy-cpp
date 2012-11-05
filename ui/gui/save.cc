#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QSettings>
#include <QtDebug>

#include "save.h"
#include "update.h"
#include "logarea.h"
#include "env.h"


////////// SaveDialog //////////

static SaveDialog *dlg;

SaveDialog::SaveDialog()
{
	resize(500, 200);
	setWindowTitle(tr("Downloads"));

	listwidget = new QWidget(this);
	listlayout = new QGridLayout(listwidget);

	listview = new LogArea(this);
	listview->setWidget(listwidget);
	listview->setWidgetResizable(true);

	QPushButton *cleanupbutton = new QPushButton(tr("Clean Up"), this);
	connect(cleanupbutton, SIGNAL(clicked()), this, SLOT(cleanup()));

	QHBoxLayout *buttonlayout = new QHBoxLayout;
	buttonlayout->addStretch();
	buttonlayout->addWidget(cleanupbutton);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(listview);
	layout->addLayout(buttonlayout);
	setLayout(layout);

	// Read the persistent download state
	Q_ASSERT(settings);
	int nsaves = settings->beginReadArray("Downloads");
	bool isact = false;
	for (int i = 0; i < nsaves; i++) {
		settings->setArrayIndex(i);
		QString path = settings->value("path").toString();
		FileInfo info = FileInfo::decode(
				settings->value("info").toByteArray());
		QString final = settings->value("final").toString();
		qDebug() << "Old download" << info.name() << "to" << path
			<< (final.isEmpty() ? "not done" : final);
		if (path.isEmpty() || info.isNull()) {
			qDebug("Invalid download record");
			continue;
		}
		(void)new SaveItem(this, info, path, final);
		if (final.isEmpty())
			isact = true;
	}
	settings->endArray();

	if (isact)
		show();
}

void SaveDialog::init()
{
	if (!dlg)
		dlg = new SaveDialog();
}

void SaveDialog::present()
{
	init();
	dlg->show();
	dlg->raise();
}

int SaveDialog::numActive()
{
	int n = 0;
	for (int i = 0; i < dlg->items.size(); i++)
		if (!dlg->items[i]->isDone())
			n++;
	return n;
}

void SaveDialog::save(const FileInfo &info, const QString &localname)
{
	qDebug() << "Save" << info.name() << "as" << localname;

	init();

	(void)new SaveItem(dlg, info, localname);
	dlg->writestate();

	dlg->show();
	dlg->raise();
}

void SaveDialog::cleanup()
{
	qDebug() << "SaveDialog::cleanup";

	for (int row = items.size()-1; row >= 0; row--) {
		SaveItem *item = items.at(row);
		if (!item->isDone())
			continue;

		items.removeAt(row);
		item->deleteLater();
	}
	writestate();
	relayout();
}

void SaveDialog::writestate()
{
	settings->beginWriteArray("Downloads", items.size());
	for (int i = 0; i < items.size(); i++) {
		SaveItem *item = items.at(i);
		settings->setArrayIndex(i);
		settings->setValue("path", item->localpath);
		settings->setValue("info", item->info.encode());
		if (item->isDone())
			settings->setValue("final", item->finalmsg);
	}
	settings->endArray();
	settings->sync();
}

void SaveDialog::relayout()
{
	// Delete the old grid layout
	Q_ASSERT(listwidget->layout() == listlayout);
	delete listlayout;

	// Install and populate a fresh layout
	listlayout = new QGridLayout(listwidget);
	int nitems = items.size();
	for (int row = 0; row < nitems; row++) {
		SaveItem *item = items.at(row);
		listlayout->addWidget(item->status, row, 0);
		listlayout->addWidget(item->progress, row, 1);
		listlayout->addWidget(item->cancel, row, 2);
	}

	listlayout->setRowMinimumHeight(nitems, 1);
	listlayout->setRowStretch(nitems, 1);
}


////////// SaveItem //////////

const int SaveItem::progressMax;

SaveItem::SaveItem(SaveDialog *dlg, const FileInfo &info,
		const QString &localpath, const QString &final)
:	QObject(dlg), info(info), localpath(localpath), finalmsg(final),
	act(NULL)
{
	status = new QLabel(dlg->listwidget);

	progress = new QProgressBar(dlg->listwidget);
	progress->setRange(0, progressMax);

	cancel = new QPushButton(tr("Cancel"), dlg->listwidget);
	connect(cancel, SIGNAL(clicked()), this, SLOT(cancelPressed()));

	int row = dlg->items.size();

	dlg->listlayout->addWidget(status, row, 0);
	dlg->listlayout->addWidget(progress, row, 1);
	dlg->listlayout->addWidget(cancel, row, 2);

	dlg->listlayout->setRowMinimumHeight(row+1, 1);
	dlg->listlayout->setRowStretch(row+1, 1);

	dlg->items.append(this);

	// Start the download
	if (final.isEmpty()) {
		qDebug() << "Start download of" << info.name()
			<< "to" << localpath;
		act = new Update(dlg, info, localpath);
		connect(act, SIGNAL(statusChanged()),
			this, SLOT(updateStatus()));
		connect(act, SIGNAL(progressChanged(float)),
			this, SLOT(updateProgress(float)));
		act->start();
	} else {
		status->setText(final);
		cancel->setText(tr("Clean Up"));
		progress->setValue(progressMax);
	}
}

SaveItem::~SaveItem()
{
	if (act)
		delete act;
	delete status;
	delete progress;
	delete cancel;
}

bool SaveItem::isDone()
{
	return !act || act->isDone();
}

void SaveItem::pausePressed()
{
}

void SaveItem::cancelPressed()
{
	qDebug() << "SaveItem::cancelPressed";

	dlg->items.removeAll(this);
	dlg->writestate();
	dlg->relayout();
	deleteLater();
}

void SaveItem::updateStatus()
{
	status->setText(act->statusString());

	if (act->isDone()) {
		finalmsg = act->statusString();
		cancel->setText(tr("Clean Up"));
		dlg->writestate();
	}
}

void SaveItem::updateProgress(float ratio)
{
	int val = qMax(0, qMin(progressMax, (int)(progressMax * ratio)));
	progress->setValue(val);
}


////////// SaveAsDialog //////////

SaveAsDialog::SaveAsDialog(const FileInfo &info)
:	info(info),
	done(false)
{
	setFileMode(AnyFile);
	setAcceptMode(AcceptSave);
	setConfirmOverwrite(true);
	setDirectory(QDir::current());

	QString defaultname = info.name();
	if (!defaultname.isEmpty())
		selectFile(defaultname);

	connect(this, SIGNAL(finished(int)), this, SLOT(gotFinished(int)));
}

void SaveAsDialog::gotFinished(int result)
{
	// Don't save multiple times if the user manages
	// to click the button multiple times (possible under X anyway)
	if (done)
		return;
	done = true;

	// Save the file
	QStringList names = selectedFiles();
	if (names.size() == 1 && result == QDialog::Accepted)
		SaveDialog::save(info, names.first());

	deleteLater();
}

void SaveAsDialog::saveAs(const FileInfo &info)
{
	(new SaveAsDialog(info))->show();
}

