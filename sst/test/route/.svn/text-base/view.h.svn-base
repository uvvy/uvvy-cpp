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
// Visualization of the simulated network
#ifndef NETVIEW_H
#define NETVIEW_H

#include <QHash>
#include <QWidget>
#include <QMainWindow>

class QLabel;


namespace SST {

class ViewWidget : public QWidget
{
	Q_OBJECT

private:
	typedef QPair<QByteArray,QByteArray> Edge;

	double vscale;
	int vxofs, vyofs;

	QByteArray selNodeId;
	QHash<QByteArray,int> selNeighbors;
	QHash<Edge,int> selEdges;
	int viewaff;

public:
	ViewWidget(QWidget *parent = NULL);

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	void setAffinity(int n);
	//void setZoom(int n);

protected:
	void paintEvent(QPaintEvent *event);
	void mousePressEvent(QMouseEvent *event);
};

class ViewWindow : public QMainWindow
{
	Q_OBJECT

private:
	ViewWidget *vwig;
	QLabel *afflabel;

public:
	ViewWindow(QWidget *parent = NULL);

private slots:
	//void zoomSliderChanged(int n);
	void setAffinity(int n);
};

} // namespace SST

inline uint qHash(const QPair<QByteArray,QByteArray> &edge)
	{ return qHash(edge.first) + qHash(edge.second); }

#endif	// NETVIEW_H
