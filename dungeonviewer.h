#ifndef DUNGEONVIEWER_H
#define DUNGEONVIEWER_H

#include <QTabWidget>
#include <QScrollArea>
#include <QStringList>
#include <QLabel>
#include <QWheelEvent>
#include <QGuiApplication>
#include <QVector>
#include <QGroupBox>
#include <QPushButton>
#include <QGraphicsOpacityEffect>
#include <math.h>
#include <QScrollBar>
#include <QPainter>
#include <QWheelEvent>
#include "focus.h"
#include "control.h"
#include "tools.h"
#include "facilitymanager.h"

#define DV_EXCLUSIVE 0x80
#define DV_FILL 0x01
#define DV_ENLIGHT 0x02

#define DV_SCROLLSTEP 5

typedef struct {
	QImage * map;
	uint8_t * wall;
	QImage * thumbnail;
	QString name;
	int width, height;
} Dungeon;

class DungeonViewer : public QWidget {
	Q_OBJECT;
enum Mode {DV_SELECT, DV_DISPLAY};
public:
	DungeonViewer(QTabWidget *);
protected:
	void paintEvent(QPaintEvent * event) override;
	void mousePressEvent(QMouseEvent * event);
	void mouseMoveEvent(QMouseEvent * event);
	void wheelEvent(QWheelEvent * event);
private:
	void readFiles();
	QList <Dungeon> dungeon;
	void setAlpha(QImage * image, int value);
	void fill(Dungeon map, QPoint f);
	void enlight(Dungeon map, QPoint f, int radius);
	QTabWidget * parent;
	QLabel * background;
	int actualmap;
	int centerx, centery;
	QPoint dragstartpos;
	int radius; 
	Mode mode;
	int targetx, targety;
	QTimer * timer;

private slots:
	void ctrlUpdate();
	void fileUpdate(int);
	void focusEvent(QByteArray);
	void timerTick();

};



#endif