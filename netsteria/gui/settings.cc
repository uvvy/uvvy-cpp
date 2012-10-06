
#include <QFont>
#include <QDateTime>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QListView>
#include <QTabWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QStringListModel>
#include <QItemSelectionModel>
#include <QHostInfo>
#include <QSettings>
#include <QtDebug>

#include "settings.h"
#include "main.h"
#include "util.h"
#include "audio.h"
#include "meter.h"
#include "os.h"

using namespace SST;


Profile *profile;
SettingsDialog *settingsdlg;

////////// Profile //////////

Profile::Profile(QWidget *parent)
:	QWidget(parent)
{
	// Read the user's persistent profile
	settings->beginGroup("Profile");
	owneredit.setText(settings->value("Name").toString());
	if (ownerName().isEmpty())
		owneredit.setText(userName());
	hostedit.setText(settings->value("Host Name").toString());
	if (hostName().isEmpty())
		hostedit.setText(QHostInfo::localHostName());
	cityedit.setText(settings->value("City").toString());
	regionedit.setText(settings->value("Region").toString());
	countryedit.setText(settings->value("Country").toString());
	birthedit.setDate(settings->value("Birth Date").toDate());
	settings->endGroup();

	connect(&owneredit, SIGNAL(editingFinished()),
		this, SLOT(writeProfile()));
	connect(&hostedit, SIGNAL(editingFinished()),
		this, SLOT(writeProfile()));
	connect(&cityedit, SIGNAL(editingFinished()),
		this, SLOT(writeProfile()));
	connect(&regionedit, SIGNAL(editingFinished()),
		this, SLOT(writeProfile()));
	connect(&countryedit, SIGNAL(editingFinished()),
		this, SLOT(writeProfile()));
	connect(&birthedit, SIGNAL(dateChanged(const QDate&)),
		this, SLOT(writeProfile()));

	QGridLayout *maingrid = new QGridLayout();
	maingrid->addWidget(new QLabel(tr("My Name"), this), 0, 0);
	maingrid->addWidget(&owneredit, 0, 1);
	maingrid->addWidget(new QLabel(tr("Host Name"), this), 1, 0);
	maingrid->addWidget(&hostedit, 1, 1);
	maingrid->addWidget(new QLabel(tr("City"), this), 2, 0);
	maingrid->addWidget(&cityedit, 2, 1);
	maingrid->addWidget(new QLabel(tr("Region"), this), 3, 0);
	maingrid->addWidget(&regionedit, 3, 1);
	maingrid->addWidget(new QLabel(tr("Country"), this), 4, 0);
	maingrid->addWidget(&countryedit, 4, 1);
	maingrid->addWidget(new QLabel(tr("Birth Date"), this), 5, 0);
	maingrid->addWidget(&birthedit, 5, 1);
	setLayout(maingrid);
}

void Profile::writeProfile()
{
	settings->beginGroup("Profile");
	settings->setValue("Name", owneredit.text());
	settings->setValue("Host Name", hostedit.text());
	settings->setValue("City", cityedit.text());
	settings->setValue("Region", regionedit.text());
	settings->setValue("Country", countryedit.text());
	settings->setValue("Birth Date", birthedit.date());
	settings->endGroup();
	settings->sync();

	// XX update RegClient registration

	profileChanged();
}

////////// Audio Control //////////

AudioControl::AudioControl(QWidget *parent)
:	QWidget(parent)
{
	aloop = new AudioLoop(this);
	connect(aloop, SIGNAL(startPlayback()), this, SLOT(loopPlayback()));

	// XX lib/audio.cc should provide dynamic models...
	QStringList inlist, outlist;
	int indfl = 0, outdfl = 0;
	for (int i = 0; i < Audio::scan(); i++) {
		if (Audio::inChannels(i) > 0) {
			if (i == Audio::defaultInputDevice())
				indfl = inlist.size();
			inlist.append(Audio::deviceName(i));
		}
		if (Audio::outChannels(i) > 0) {
			if (i == Audio::defaultOutputDevice())
				outdfl = outlist.size();
			outlist.append(Audio::deviceName(i));
		}
	}
	QStringListModel *inmod = new QStringListModel(this);
	QStringListModel *outmod = new QStringListModel(this);
	inmod->setStringList(inlist);
	outmod->setStringList(outlist);

	QLabel *inlabel = new QLabel(tr("Input Device"), this);
	//indefault = new QCheckBox(tr("System Default"), this);
	inview = new QListView(this);
	inview->setModel(inmod);
	inview->selectionModel()->select(inmod->index(indfl,0),
					QItemSelectionModel::SelectCurrent);
	connect(inview->selectionModel(),
		SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
		this, SLOT(inChanged(const QModelIndex&)));

	QLabel *inmeterlabel = new QLabel(tr("Input Level"), this);
	inmeter = new Meter(this);
	inmeter->setRange(0, 100);

	QVBoxLayout *inlayout = new QVBoxLayout();
	inlayout->addWidget(inlabel);
	//inlayout->addWidget(indefault);
	inlayout->addWidget(inview);
	inlayout->addSpacing(10);
	inlayout->addWidget(inmeterlabel);
	inlayout->addWidget(inmeter);

	QLabel *outlabel = new QLabel(tr("Output Device"), this);
	//outdefault = new QCheckBox(tr("System Default"), this);
	outview = new QListView(this);
	outview->setModel(outmod);
	outview->selectionModel()->select(outmod->index(outdfl,0),
					QItemSelectionModel::SelectCurrent);
	connect(outview->selectionModel(),
		SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
		this, SLOT(outChanged(const QModelIndex&)));

	QLabel *outmeterlabel = new QLabel(tr("Output Level"), this);
	outmeter = new Meter(this);
	outmeter->setRange(0, 100);

	QVBoxLayout *outlayout = new QVBoxLayout();
	outlayout->addWidget(outlabel);
	//outlayout->addWidget(outdefault);
	outlayout->addWidget(outview);
	outlayout->addSpacing(10);
	outlayout->addWidget(outmeterlabel);
	outlayout->addWidget(outmeter);

	// Input/Output layout
	QHBoxLayout *inoutlayout = new QHBoxLayout();
	inoutlayout->addLayout(inlayout);
	inoutlayout->addLayout(outlayout);

	// Loopback test area
	loopbox = new QCheckBox(tr("Audio Loopback Test"), this);
	loopbox->setToolTip(tr("When enabled, you should hear whatever you say "
				"echoed back to you after 5 seconds."));
	connect(loopbox, SIGNAL(stateChanged(int)), this, SLOT(loopChanged()));

	looplabel = new QLabel(this);
	QFont font = looplabel->font();
	font.setBold(true);
	looplabel->setFont(font);

	QHBoxLayout *looplayout = new QHBoxLayout();
	looplayout->addWidget(loopbox);
	looplayout->addStretch();
	looplayout->addWidget(looplabel);

	// Master layout
	QVBoxLayout *toplayout = new QVBoxLayout();
	toplayout->addLayout(inoutlayout);
	toplayout->addSpacing(10);
	toplayout->addLayout(looplayout);
	setLayout(toplayout);
}

void AudioControl::showEvent(QShowEvent *)
{
	// Connect these signals only while showing the audio controller,
	// because they cause the audio device to be held open.
	connect(Audio::instance(), SIGNAL(inputLevelChanged(int)),
		inmeter, SLOT(setValue(int)));
	connect(Audio::instance(), SIGNAL(outputLevelChanged(int)),
		outmeter, SLOT(setValue(int)));

	if (loopbox->isChecked())
		loopEnable();
}

void AudioControl::hideEvent(QHideEvent *)
{
	disconnect(Audio::instance(), SIGNAL(inputLevelChanged(int)),
		inmeter, SLOT(setValue(int)));
	disconnect(Audio::instance(), SIGNAL(outputLevelChanged(int)),
		outmeter, SLOT(setValue(int)));

	loopDisable();
}

void AudioControl::inChanged(const QModelIndex &cur)
{
	QString devname = inview->model()->data(cur).toString();
	qDebug() << "Change input device to" << devname;
	Audio::setInputDevice(devname);
}

void AudioControl::outChanged(const QModelIndex &cur)
{
	QString devname = outview->model()->data(cur).toString();
	qDebug() << "Change output device to" << devname;
	Audio::setOutputDevice(devname);
}

void AudioControl::loopChanged()
{
	if (!isVisible())
		return;

	if (loopbox->isChecked())
		loopEnable();
	else
		loopDisable();
}

void AudioControl::loopEnable()
{
	aloop->enable();
	looplabel->setText(tr("Recording..."));
}

void AudioControl::loopDisable()
{
	aloop->disable();
	looplabel->setText(QString());
	inmeter->setValue(0);
	outmeter->setValue(0);
}

void AudioControl::loopPlayback()
{
	if (aloop->enabled())
		looplabel->setText(tr("Playing and Recording..."));
}

////////// SettingsDialog //////////

SettingsDialog::SettingsDialog()
{
	setWindowTitle(tr("Netsteria Settings"));

	tabwidget = new QTabWidget(this);

	profile = new Profile();
	AudioControl *audio = new AudioControl();

	tabwidget->addTab(profile, tr("Profile"));
	tabwidget->addTab(audio, tr("Audio"));

	QGridLayout *layout = new QGridLayout();
	layout->addWidget(tabwidget, 0, 0);
	setLayout(layout);
}

void SettingsDialog::init()
{
	if (!settingsdlg)
		settingsdlg = new SettingsDialog();
}

void SettingsDialog::openSettings()
{
	init();
	settingsdlg->show();
	settingsdlg->raise();
	settingsdlg->activateWindow();
}

void SettingsDialog::openProfile()
{
	openSettings();
	settingsdlg->tabwidget->setCurrentIndex(0);
}

