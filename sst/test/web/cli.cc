/*
 * Structured Stream Transport
 * Copyright (C) 2006-2008 Massachusetts Institute of Technology
 * Author: Bryan Ford
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <netinet/in.h>

#include <QDir>
#include <QFile>
#include <QIcon>
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QScrollBar>
#include <QToolBar>
#include <QTextDocument>
#include <QMouseEvent>
#include <QUrl>

#include "cli.h"

using namespace SST;


// Number of parallel downloads for legacy mode - 2 is RFC-compliant behavior.
static const int legacyConns = 2;


////////// WebView //////////

void WebView::mouseMoveEvent(QMouseEvent *ev)
{
	//qDebug() << "mouse move:" << ev->x() << ev->y();

	QTextEdit::mouseMoveEvent(ev);
	mouseMoved(ev->x(), ev->y());
}



////////// WebClient //////////

WebClient::WebClient(Host *host, const QHostAddress &srvaddr, int srvport,
			SimLink *link)
:	host(host), srvaddr(srvaddr), srvport(srvport), link(link),
	refresher(host)
{
	resize(1000, 700);

	QToolBar *toolbar = addToolBar(tr("Browser Controls"));
	toolbar->addAction(QIcon("images/forward.png"), tr("Forward")); 
	toolbar->addAction(QIcon("images/back.png"), tr("Back"));
	toolbar->addAction(QIcon("images/reload.png"), tr("Reload"),
						this, SLOT(reload()));

	priobox = new QComboBox(this);
	priobox->addItem(tr("Traditional Web Browser Behavior"));
	priobox->addItem(tr("Prioritize Visible Images"));
	priobox->addItem(tr("Prioritize Image Under Pointer"));
	toolbar->addWidget(priobox);
	connect(priobox, SIGNAL(currentIndexChanged(int)),
		this, SLOT(prioboxChanged(int)));

	// We only get a speed-throttle control on the simulated network.
	if (link) {
		QSlider *speedslider = new QSlider(this);
		speedslider->setMinimum(10);	// 2^10 = 1024 bytes per sec
		speedslider->setMaximum(30);	// 2^30 = 1 GByte per sec
		speedslider->setValue(13);	// 2^13 = 8KB/sec
		speedslider->setOrientation(Qt::Horizontal);
		toolbar->addWidget(speedslider);

		speedlabel = new QLabel(this);
		toolbar->addWidget(speedlabel);

		connect(speedslider, SIGNAL(valueChanged(int)),
			this, SLOT(speedSliderChanged(int)));
		speedSliderChanged(speedslider->value());
	}

	QFile pagefile("page/index.html");
	if (!pagefile.open(QIODevice::ReadOnly))
		qFatal("can't open web page");
	QString html = QString::fromAscii(pagefile.readAll());

	webview = new WebView(this);
	webview->setReadOnly(true);
	connect(webview, SIGNAL(mouseMoved(int,int)),
		this, SLOT(mouseMoved(int,int)));

	QTextDocument *doc = webview->document();
	doc->setHtml(html);

	// Find all the images in the web page, and set their true sizes.
	// This cheat is just because I'm too lazy to add width=, height=
	// fields to all the img markups in my photo album pages...
	QTextCursor curs(doc);
	QString imgchar = QString(QChar(0xfffc));
	while (true) {
		curs = doc->find(imgchar, curs);
		if (curs.isNull())
			break;

		// Get the image's format information
		QTextFormat fmt = curs.charFormat();
		if (!fmt.isImageFormat()) {
			qDebug() << "image character isn't image!?";
			continue;
		}
		QTextImageFormat ifmt = fmt.toImageFormat();
		//qDebug() << "image" << ifmt.name() << "width" << ifmt.width()
		//		<< "height" << ifmt.height();
		QString name = ifmt.name();

		// Look at the actual image file to get the image's size.
		// (This is where the horrible cheat occurs.)
		QString pagename = "page/" + name;
		QImage img(pagename);
		if (img.isNull()) {
			qDebug() << "couldn't find" << ifmt.name();
			continue;
		}
		ifmt.setWidth(img.width());
		ifmt.setHeight(img.height());
		//ifmt.setName("images/blank.png");
		//ifmt.setName("page/" + ifmt.name());
		curs.setCharFormat(ifmt);

		// Record the name and text position of this image.
		WebImage wi;
		wi.curs = curs;
		wi.name = ifmt.name();
		wi.imgsize = QFile(pagename).size();
		images.append(wi);
	}

#if 0
	// Now that all the image sizes are known
	// and the document is hopefully formatted correctly,
	// find where all the images appear in the viewport.
	for (int i = 0; i < images.size(); i++) {
		WebImage &wi = images[i];
		QTextCursor curs(doc);

		curs.setPosition(wi.curs.selectionStart());
		QRect rbef = webview->cursorRect(curs);
		curs.setPosition(wi.curs.selectionEnd());
		QRect raft = webview->cursorRect(curs);
		wi.rect = rbef.united(raft);
		qDebug() << "image" << i << "rect" << wi.rect;
	}
#endif

	webview->moveCursor(QTextCursor::Start);

	setCentralWidget(webview);

	// Watch the text edit box's vertical scroll bar for changes.
	connect(webview->verticalScrollBar(), SIGNAL(valueChanged(int)),
		this, SLOT(refreshSoon()));

	connect(&refresher, SIGNAL(timeout(bool)), this, SLOT(refreshNow()));

	// Start loading the images on this page
	reload();
}

void WebClient::reload()
{
	// Open a stream and send a request for each image.
	for (int i = 0; i < images.size(); i++) {
		WebImage &wi = images[i];

		// Delete any previous stream and temp file
		if (wi.strm) {
			Q_ASSERT(strms.value(wi.strm) == i);
			strms.remove(wi.strm);
			wi.strm->shutdown(Stream::Reset);
			wi.strm->deleteLater();
			wi.strm = NULL;
		}
		if (wi.tmpf) {
			wi.tmpf->remove();
			delete wi.tmpf;
			wi.tmpf = NULL;
		}

		// Reset the visible image to the special blank image
		QTextImageFormat ifmt = wi.curs.charFormat().toImageFormat();
		ifmt.setName("images/black.png");
		wi.curs.setCharFormat(ifmt);

		// Create a temporary file to load the image into
		wi.tmpf = new QFile("tmp/" + wi.name, this);
		QDir().mkpath(wi.tmpf->fileName());
		QDir().rmdir(wi.tmpf->fileName());
		if (!wi.tmpf->open(QIODevice::WriteOnly | QIODevice::Truncate))
			qFatal("Can't write tmp image");

		// Send the image request immediately for prioritization,
		// or only for the first two images for legacy behavior.
		if (priobox->currentIndex() > 0 || i < legacyConns)
			sendRequest(i);

		wi.done = false;
	}

	refreshSoon();
}

void WebClient::sendRequest(int img)
{
	WebImage &wi = images[img];
	if (wi.strm != NULL)
		return;

	// Create an SST stream on which to download the image
	wi.strm = new Stream(host, this);
	connect(wi.strm, SIGNAL(readyRead()), this, SLOT(readyRead()));
	strms.insert(wi.strm, img);

	// Connect to the server and send the request
	wi.strm->connectTo(Ident::fromIpAddress(srvaddr, srvport),
				"webtest", "basic");
	wi.strm->writeMessage(wi.name.toAscii());
}

void WebClient::setPriorities()
{
	//qDebug() << "setPriorities";

	if (priobox->currentIndex() <= 0)
		return;		// No prioritization!
	bool ptrpri = (priobox->currentIndex() == 2);

	int start = webview->cursorForPosition(QPoint(0,0)).position();
	int end = webview->cursorForPosition(
				QPoint(webview->width(),webview->height()))
			.position();
	//qDebug() << "window:" << start << "to" << end;

	for (int i = 0; i < images.size(); i++) {
		WebImage &wi = images[i];

		// Determine if this image is currently visible
		bool vis = wi.curs.selectionStart() <= end &&
				wi.curs.selectionEnd() >= start;
		int pri = vis ? 1 : 0;

		// If it's visible, see if it's under the mouse pointer
		// and prioritize it further if appropriate.
		if (vis && ptrpri) {
			QTextCursor curs(webview->document());
			curs.setPosition(wi.curs.selectionStart());
			QRect rbef = webview->cursorRect(curs);
			curs.setPosition(wi.curs.selectionEnd());
			QRect raft = webview->cursorRect(curs);
			QRect rect = rbef.united(raft);
			if (rect.contains(mousex, mousey))
				pri = 2;

			//qDebug() << "ptr" << mousex << mousey
			//	<< (pri == 2 ? "in" : "not in")
			//	<< rect << "img" << i;
		}

		// Set the image's priority accordingly
		setPri(i, pri);
	}
}

void WebClient::setPri(int i, int pri)
{
	WebImage &wi = images[i];
	if (wi.strm == NULL || wi.pri == pri)
		return;		// no change

	qDebug() << "change img" << i << "to priority" << pri;

	// Send a priority change message
	quint32 msg = htonl(pri);
	wi.strm->writeDatagram((char*)&msg, sizeof(msg), true);

	wi.pri = pri;
}

void WebClient::prioboxChanged(int sel)
{
	webview->trackMouse(sel == 2);
	qDebug() << "mouse tracking" << (sel == 2);

	refreshSoon();
}

void WebClient::readyRead()
{
	Stream *strm = (Stream*)sender();
	Q_ASSERT(strms.contains(strm));
	int win = strms.value(strm);
	WebImage &wi = images[win];
	Q_ASSERT(wi.strm == strm);

	while (true) {
		QByteArray buf;
		buf.resize(wi.strm->bytesAvailable());
		int act = wi.strm->readData(buf.data(), buf.size());
		if (act <= 0) {
			if (wi.strm->atEnd()) {
				// Done loading this image!
				done:
				qDebug() << "img" << win << "done";
				wi.strm->deleteLater();
				wi.strm = NULL;
				wi.done = true;
			}
			return;
		}

		// Write the image data to the temporary file
		wi.tmpf->write(buf, act);
		wi.tmpf->flush();

		qDebug() << "image" << win << "got" << wi.tmpf->pos()
			<< "of" << wi.imgsize << "bytes";

		// Update the image in the browser window ASAP
		wi.dirty = true;
		refreshSoon();

		// XX shouldn't be necessary!
		if (wi.tmpf->pos() >= wi.imgsize)
			goto done;
	}
}

void WebClient::refreshSoon()
{
	if (!refresher.isActive())
		refresher.start(100*1000);	 // 1/10th sec
}

void WebClient::refreshNow()
{
	//qDebug() << "refresh";
	refresher.stop();

	// Send priority change requests
	setPriorities();

	// Update on-screen images
	for (int i = 0; i < images.size(); i++) {
		WebImage &wi = images[i];

		if (!wi.dirty)
			continue;
		wi.dirty = false;

		// Load the image from disk
		QImage img("tmp/" + wi.name);
		webview->document()->addResource(QTextDocument::ImageResource,
						QUrl(wi.name), img);
		QTextImageFormat ifmt = wi.curs.charFormat().toImageFormat();
		ifmt.setName(wi.name);
		wi.curs.setCharFormat(ifmt);
	}

	// Legacy: fire off new requests after images complete
	// Prio: fire all requests for ALL images not started loading yet
	int maxreqs = priobox->currentIndex() > 0 ? 9999 : legacyConns;
	int ncur = 0;
	for (int i = 0; i < images.size(); i++) {
		WebImage &wi = images[i];
		if (wi.done)
			continue;
		if (ncur >= maxreqs)
			break;
		if (wi.strm == NULL)
			sendRequest(i);
		ncur++;
	}
}

void WebClient::speedSliderChanged(int value)
{
	Q_ASSERT(value >= 0 && value <= 30);
	int bw = 1 << value;

	qDebug() << "change network bandwidth:" << bw;

	link->setLinkRate(bw);

	QString str;
	if (value >= 30)
		str = tr("%0 GBytes/sec").arg(1 << (value-30));
	else if (value >= 20)
		str = tr("%0 MBytes/sec").arg(1 << (value-20));
	else if (value >= 10)
		str = tr("%0 KBytes/sec").arg(1 << (value-10));
	else
		str = tr("%0 Bytes/sec").arg(1 << value);
	speedlabel->setText(str);
}

void WebClient::mouseMoved(int x, int y)
{
	mousex = x;
	mousey = y;
	refreshSoon();
}

