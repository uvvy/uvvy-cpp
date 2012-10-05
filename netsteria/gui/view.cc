
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QToolButton>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMouseEvent>
#include <QMenu>
#include <QFileDialog>
#include <QApplication>
#include <QLocale>
#include <QtDebug>

#include "view.h"
#include "share.h"
#include "save.h"


////////// Viewer //////////

static QPixmap *filePixmap;
static QPixmap *dirOpenPixmap;
static QPixmap *dirClosedPixmap;
static QPixmap *filelinkPixmap;

static void loadIcons()
{
	if (filePixmap)
		return;
	filePixmap = new QPixmap("img/file-16.png");
	dirOpenPixmap = new QPixmap("img/diropen-16.png");
	dirClosedPixmap = new QPixmap("img/dirclosed-16.png");
	filelinkPixmap = new QPixmap("img/filelink-16.png");
}

Viewer::Viewer(QWidget *parent, const FileInfo &info)
:	QWidget(parent),
	info(info),
	content(NULL)
{
	// Top-level layout is a 2x2 grid.
	layout = new QGridLayout();
	layout->setMargin(0);
	layout->setSpacing(0);

	// Type-indicator icon: top-left
	loadIcons();
	const QPixmap *pixmap = info.isFile()	? filePixmap
			: info.isDirectory()	? dirClosedPixmap
			: info.isSymLink()	? filelinkPixmap
						: filePixmap;	// XXX
	typelabel = new QLabel(this);
	typelabel->setPixmap(*pixmap);
	QFont typefont = typelabel->font();
	typefont.setBold(true);
	typelabel->setFont(typefont);
	layout->addWidget(typelabel, 0, 0);

	// Header line: top-right
	QHBoxLayout *hdrlayout = new QHBoxLayout();
	hdrlayout->setMargin(0);
	layout->addLayout(hdrlayout, 0, 1);

	// Name: left justified in header line
	QLabel *namelabel = new QLabel(info.name(), this);
	namelabel->setFrameStyle(QFrame::NoFrame);
	hdrlayout->addWidget(namelabel);

	// Stats (e.g., file size): right justified in header line
	hdrlayout->addStretch();	// right-align the rest
	if (info.isFile()) {
		//QLabel *sizeLabel = new QLabel(
		//	bytesNumber(info.fileSize()), this);
		QLabel *sizeLabel = new QLabel(
			QLocale().toString(info.fileSize()) + tr(" bytes"),
			this);
		hdrlayout->addWidget(sizeLabel);
	}

	// Content area: lower-right grid area.
	if (info.isDirectory()) {
		content = new DirView(this, info);
		content->hide();
		layout->addWidget(content, 1, 1);
	}

	setLayout(layout);
}

void Viewer::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		pressPos = event->pos();
	else if (event->button() == Qt::RightButton)
		showMenu(event->pos());
}

void Viewer::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton)
		return;
	if ((event->pos() - pressPos).manhattanLength()
			>= QApplication::startDragDistance())
		return;		// It's a drag, not a click

	if (typelabel->rect().contains(event->pos()) && info.isDirectory()) {

		// User clicked on the directory type icon -
		// open or close the directory listing.
		if (content->isVisible()) {
			content->hide();
			typelabel->setPixmap(*dirClosedPixmap);
		} else {
			content->show();
			typelabel->setPixmap(*dirOpenPixmap);
		}
		return;
	}

	// Show the context menu
	showMenu(event->pos());
}

#if 0
void Viewer::mouseMoveEvent(QMouseEvent *event)
{
	if (!(event->buttons() & Qt::LeftButton))
		return;
	if ((event->pos() - pressPos).manhattanLength()
			< QApplication::startDragDistance())
		return;

	QDrag *drag = new QDrag(this);
	QMimeData *mimeData = new QMimeData;

	mimeData->setData(mimeType, data);
	drag->setMimeData(mimeData);

	Qt::DropAction dropAction = drag->start(Qt::CopyAction | Qt::MoveAction);
	...
}
#endif

void Viewer::showMenu(const QPoint &pos)
{
	// XX allow selecting just one file or all files in box...
	QMenu menu(tr("FOO!"), this);
	QAction *save = menu.addAction(tr("Save As..."));
	menu.addAction(tr("Cancel"));
	if (menu.exec(mapToGlobal(pos)) == save)
		saveAs();
}

void Viewer::saveAs()
{
	SaveAsDialog::saveAs(info);
}


////////// DirView //////////

DirView::DirView(QWidget *parent, const FileInfo &dirInfo)
:	QWidget(parent),
	AbstractDirReader(NULL)
{
	layout = new QVBoxLayout();
	layout->setSpacing(5);
	layout->setMargin(3);
	setLayout(layout);

	readDir(dirInfo);
}

void DirView::gotEntries(int pos, qint64,
			const QList<FileInfo> &ents)
{
	for (int i = 0; i < ents.size(); i++) {
		Viewer *v = new Viewer(this, ents[i]);
		layout->insertWidget(pos+i, v);
	}
}

void DirView::noEntries(int, qint64, int)
{
	// XX indicate somehow
}

