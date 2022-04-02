#ifndef MAPVIEWER_H
#define MAPVIEWER_H
#include "control.h"
#include <QTabWidget>
#include <QScrollArea>
#include <QStringList>
#include <QLabel>
#include <QWheelEvent>
#include <QGuiApplication>
#include <math.h>
#include "focus.h"
#include "facilitymanager.h"
#include "bulktransfer.h"
#include "convert.h"

class Map : public QWidget {
	Q_OBJECT
public:
	Map(QString, QTabWidget *);
	void wheelEvent(QWheelEvent *);
	void resizeEvent(QResizeEvent * event);
	void mousePressEvent(QMouseEvent * event);
	void mouseMoveEvent(QMouseEvent * event);
private:
	void viewUpdate();
	VisibleButton * visibleButton;
	FocusButton * focusButton;
	QPixmap * map;
	QLabel * label;
	float zoom;
	float zoomxcenter, zoomycenter;
	QPoint dragstartpos;
	QString name;
	QTabWidget * parent;
private slots:
	void visibleButtonToggled(QString name, bool status);
	void focusButtonPressed(QString name);
public slots:
	void focusEvent(QByteArray);
};

class MapViewer : public QWidget {
	Q_OBJECT
public:
	MapViewer(QTabWidget *);
private:
	QStringList mapList;
	QTabWidget * parent;
	QList<int> indexList;
	void readFiles();
public slots:
	void ctrlUpdate();
	void fileUpdate(int);
	void viewUpdate();
};


#endif