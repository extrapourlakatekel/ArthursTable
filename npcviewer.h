#ifndef NPCVIEWER_H
#define NPCVIEWER_H

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
#include "focus.h"
#include "control.h"
#include "tools.h"
#include "facilitymanager.h"


class Npc : public QWidget {
	Q_OBJECT;
public:
	Npc(QString, QWidget *);
	void setIcon(QString);
	void resizeEvent(QResizeEvent *);
	void setPortrait(QString);
	void setText(QString);
	void setSecret(QString);
	void viewUpdate();
	QString getName();
private:
	VisibleButton * visibleButton;
	FocusButton * focusButton;
	QGroupBox * frame;
	QPixmap * portrait;
	QPixmap * invisible;
	QGraphicsOpacityEffect * opacityEffect;

	QLabel * textLabel, * picLabel, * overlayLabel;
	QPushButton * focus, * visible;
	QString n;
private slots:
	void visibleButtonToggled(QString, bool);
	void focusButtonPressed(QString);
};

class NpcViewer : public QScrollArea {
	Q_OBJECT;
public:
	NpcViewer(QTabWidget *);
	//void wheelEvent(QWheelEvent *);
	void resizeEvent(QResizeEvent * event);
	//void mousePressEvent(QMouseEvent * event);
	//void mouseMoveEvent(QMouseEvent * event);
private:
	QVector<Npc*> npcs;
	bool configure();
	void update();
	QWidget * frame;
	QPixmap * map;
	QLabel * label;
	float zoom;
	float zoomxcenter, zoomycenter;
	QPoint dragstartpos;
	QTabWidget * parent;
	int xpos;
public slots:
	void ctrlUpdate();
	void fileUpdate(int dir, QString filename);
	void viewUpdate();
	void focusEvent(QByteArray);
};


#endif