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
#ifndef CLI_H
#define CLI_H

#include <QRect>
#include <QList>
#include <QHash>
#include <QString>
#include <QMainWindow>
#include <QTextCursor>
#include <QTextEdit>

#include "host.h"
#include "sim.h"

class QFile;
class QLabel;
class QComboBox;


namespace SST {

class Host;
class Stream;


struct WebImage
{
	QTextCursor curs;
	int before, after;
	QString name;
	//QRect rect;
	int imgsize;

	Stream *strm;
	int pri;
	QFile *tmpf;
	bool dirty;
	bool done;

	inline WebImage()
		: strm(NULL), pri(0), tmpf(NULL) { }
};

class WebView : public QTextEdit
{
	Q_OBJECT

public:
	inline WebView(QWidget *parent) : QTextEdit(parent) { }

	inline void trackMouse(bool track) { setMouseTracking(track); }

signals:
	void mouseMoved(int x, int y);

private:
	void mouseMoveEvent(QMouseEvent *event);
};

class WebClient : public QMainWindow
{
	Q_OBJECT

private:
	Host *host;
	QHostAddress srvaddr;
	int srvport;
	SimLink *link;

	QComboBox *priobox;
	QLabel *speedlabel;
	WebView *webview;

	QList<WebImage> images;
	QHash<Stream*,int> strms;

	Timer refresher;

	int mousex, mousey;	// for prioritizing image under pointer

public:
	WebClient(Host *host, const QHostAddress &srvaddr, int srvport,
			SimLink *link = NULL);

private:
	void sendRequest(int img);
	void setPri(int img, int pri);

private slots:
	void prioboxChanged(int);
	void reload();
	void setPriorities();
	void readyRead();
	void refreshSoon();
	void refreshNow();
	void speedSliderChanged(int value);
	void mouseMoved(int x, int y);
};


} // namespace SST

#endif	// CLI_H
