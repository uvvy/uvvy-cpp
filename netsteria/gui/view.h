#ifndef VIEW_H
#define VIEW_H

#include <QWidget>

#include "file.h"

class QLabel;
class QVBoxLayout;
class QGridLayout;


class Viewer : public QWidget
{
	FileInfo info;
	QLabel *typelabel;
	QWidget *content;
	QGridLayout *layout;
	QPoint pressPos;

public:
	Viewer(QWidget *parent, const FileInfo &info);

private:
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

	void showMenu(const QPoint &pos);
	void saveAs();
};


class DirView : public QWidget, public AbstractDirReader
{
	QVBoxLayout *layout;

public:
	DirView(QWidget *parent, const FileInfo &dirInfo);

private:
	void gotEntries(int pos, qint64 recno,
				const QList<FileInfo> &ents);
	void noEntries(int pos, qint64 recno, int nents);
};

#endif	// VIEW_H
