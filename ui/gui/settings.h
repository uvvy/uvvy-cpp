#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include <QLineEdit>
#include <QDateEdit>

class QDateTime;
class QTabWidget;
class QListView;
class QCheckBox;
class QLabel;
class QModelIndex;
class Meter;
class AudioLoop;


class Profile : public QWidget
{
	Q_OBJECT

private:
	QLineEdit owneredit, hostedit;
	QLineEdit cityedit, regionedit, countryedit;
	QDateEdit birthedit;

public:
	Profile(QWidget *parent = NULL);

	inline QString ownerName() { return owneredit.text(); }
	inline QString hostName() { return hostedit.text(); }
	inline QString city() { return cityedit.text(); }
	inline QString region() { return regionedit.text(); }
	inline QString country() { return countryedit.text(); }
	inline QDate birthDate() { return birthedit.date(); }

signals:
	void profileChanged();

private slots:
	void writeProfile();
};

class AudioControl : public QWidget
{
	Q_OBJECT

private:
	AudioLoop *aloop;

	QCheckBox *indefault, *outdefault;
	QListView *inview, *outview;
	Meter *inmeter, *outmeter;
	QCheckBox *loopbox;
	QLabel *looplabel;

protected:
	void showEvent(QShowEvent *event);
	void hideEvent(QHideEvent *event);

public:
	AudioControl(QWidget *parent = NULL);

private:
	void loopEnable();
	void loopDisable();

private slots:
	void inChanged(const QModelIndex &current);
	void outChanged(const QModelIndex &current);
	void loopChanged();
	void loopPlayback();
};

class SettingsDialog : public QDialog
{
	Q_OBJECT

private:
	QTabWidget *tabwidget;

public:
	SettingsDialog();

	static void init();
	static void openSettings();
	static void openProfile();
};

extern Profile *profile;
extern SettingsDialog *settingsdlg;

#endif	// SETTINGS_H
