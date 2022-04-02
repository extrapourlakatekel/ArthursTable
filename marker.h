#ifndef MARKER_H
#define MARKER_H
#include <inttypes.h>
//#include <malloc.h>
#include <QAction>
#include <QActionGroup>
#include <QContextMenuEvent>
#include <QDebug>
#include <QGuiApplication>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QToolButton>
#include <QResizeEvent>
#include <QWidget>
#include <QWidgetAction>
#include <wchar.h>
//#include "algorithms.h"
#include "control.h"
#include "convert.h"
#include "config.h"
#include "tools.h"

#define MARKER_SIZE 21 // Odd numbers make prettier circle centers
#define MARKER_COLORS 10
const QColor markerPal[MARKER_COLORS] = {Qt::white, Qt::gray, Qt::red, QColor::fromRgb(255,128,0), Qt::yellow, Qt::green, QColor::fromRgb(0,255,255), 
								   Qt::blue, QColor::fromRgb(255,0,255), QColor::fromRgb(184,134,11)};

#define MARKER_STYLES 6
enum MarkerStyle {MARKER_TEAM, MARKER_CIRCLE, MARKER_SQUARE, MARKER_TRIANGLE, MARKER_HEXAGON, MARKER_CROSS};

class Marker : public QLabel
{
	Q_OBJECT
public:
	Marker(int id, QWidget *parent);
	~Marker();
	void setStyle(MarkerStyle, int);
	void updateZoom(float zoom, float zoomxcenter, float zoomycenter);
	void updatePosition();
	void updateBorder(int width, int height);
	static void draw(QPainter * painter, MarkerStyle style, int colorindex, int animator = 0);
	QPoint getPos();
	void setAbsPos(int x, int y);
	void setRelPos(int x, int y);
	QByteArray getConfig();
	void setConfig(QByteArray *);
	int xpos, ypos;
	uint8_t team;
	bool active;
protected:
	QCursor cursor;
	void paintEvent(QPaintEvent * event);
	void mousePressEvent(QMouseEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);
	void mouseMoveEvent(QMouseEvent * event);
    void contextMenuEvent(QContextMenuEvent *event) override;
private:
	QWidget * parent;
	QAction * deleteAct;
	QTimer timer;
	MarkerStyle style;
	uint8_t colornr;
	int id;
	int areawidth, areaheight;
	float zoom; // The zoom level of the image we're on
	float zoomxcenter; // The zoom center of the image we're on
	float zoomycenter;
private slots:
	void timerTick();
	void deletePressed();
signals:
	void dragged(uint8_t); // Marker was repositioned
	void deleted(uint8_t);
};

class MarkerSelection : public QWidgetAction
{
	Q_OBJECT
public: 
	MarkerSelection(QObject* parent = NULL);
protected:
	QWidget *createWidget(QWidget * parent) override;
private:
	QToolButton * button[MARKER_STYLES][MARKER_COLORS];
	QWidget * parent;
private slots:
	void buttonPressed();
signals:
	void newMarker(int, int);
};

#endif