#ifndef SKETCHPAD_H
#define SKETCHPAD_H

#include <QCloseEvent>
#include <QComboBox>
#include <QScrollArea>
#include <QStringList>
#include <QLabel>
#include <QWheelEvent>
#include <QGuiApplication>
#include <math.h>
#include <QTimer>
#include <QDialog>
#include <QPaintEvent>
#include <QPainter>
#include <QTabWidget>
#include <QGroupBox>
#include <QBrush>
#include <QCursor>
#include <QButtonGroup>
#include <QToolButton>
#include <QImage>
#include <QPushButton>
#include <inttypes.h>
#include <QMessageBox>
#include "bulktransfer.h"
#include "control.h"
#include "convert.h"

#define SKETCHPAGES 10
#define SKETCHCOLORS 40
#define SKETCHWIDTHS 5
#define DELETED 0x8000
#define CLEARALL 0xFFFF
#define BACKGROUND 0xFFFE
#define OFFSET 0xFFFD


class CourtYard {
public:
	CourtYard();
	void expand(int x, int y);
	void reset();
	int x1, x2, y1, y2;
};

class SketchTools : public QGroupBox
{
	Q_OBJECT
public:
	SketchTools(QWidget * parent);
	int getColor();
	int getWidth();
	void registerNewTab(QString * filename, uint16_t * scale, uint16_t * brightness);
public slots:
	void gamemasterChange();
private:
	QPushButton * colorButton[SKETCHCOLORS];
	QPushButton * widthButton[SKETCHWIDTHS];
	QButtonGroup * colorButtonGroup;
	QButtonGroup * widthButtonGroup;
	QToolButton * clearButton;
	QToolButton * undoButton;
	QComboBox * backgroundFileBox;
	QComboBox * backgroundScalingBox;
	QComboBox * backgroundBrightnessBox;
	QString * backgroundFile = NULL;
	uint16_t * backgroundScale = NULL;
	uint16_t * backgroundBrightness = NULL;
private slots:
	void clearButtonPressed();
	void undoButtonPressed();
	void updateBackgroundSettings();
public slots:
	void fileUpdate(int dir, QString filename);
signals:
	void clearActualPage();
	void undo();
	void updateBackground();
};

class SketchElement
{
public:
	SketchElement();
	QByteArray pack();
	void clear();
	bool unpack(QByteArray * in);
	uint16_t type;
	uint16_t properties;
	int16_t x1, y1, x2, y2;
	QString extra;
	bool operator==(const SketchElement& se) {
	return this->type       == se.type && 
		   this->properties == se.properties &&
		   this->x1         == se.x1 &&
		   this->x2         == se.x2 &&
		   this->y1         == se.y1 &&
		   this->y2         == se.y2 &&
		   this->extra      == se.extra;
	}
};

class SketchPage : public QWidget
{
	Q_OBJECT
public:
	SketchPage(SketchTools * tools, int nr, QWidget * parent);
	~SketchPage();
	void newSketch(int player, QByteArray data);
	void clear();
	void isActiveTab();
protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);
	void mouseMoveEvent(QMouseEvent * event);
	void wheelEvent(QWheelEvent * event);
	QCursor cursor;
private:
	QVector <SketchElement> sketchElement;
	QByteArray sendBuffer[MAXPLAYERLINKS];
	//QVector <SketchElement> remoteElement[MAXPLAYERLINKS];
	QVector <SketchElement> undoList;
	SketchElement * activeElement = NULL; // Might become a vector in the future
	int mynr;
	bool drawstraight;
	int32_t xos, yos;
	int lastx, lasty;
	void draw(QPainter *, SketchElement *);
	void eraser(int x, int y, int eraseradius);
	bool getCutPositions(float &p1, float &p2, int x1, int y1, int x2, int y2, int xc, int yc, int rc);
	void store();
	void load();
	void scrollLeft();
	void scrollRight();
	void scrollUp();
	void scrollDown();
	void addRemote(SketchElement *element);
	void removeRemote(SketchElement *element);
	void addLocal(SketchElement * element);
	QByteArray getSnapshot();
	void lock();
	void unlock();
	QTimer timer;
	CourtYard * courtYard;
	SketchTools * sketchTools;
	QString backgroundFile;
	uint16_t backgroundScale, backgroundBrightness;
	QImage backgroundImage;
	bool erase;
	int penColor;
	int penWidth;
	bool scroll;
	bool locked;
	bool paintingInProgress;
	bool fast;
private slots:
	void timerTick();
public slots:
	void connectionStateChange(int);
	void gamemasterChange();
	void fileUpdate(int, QString);
	void undo();
	void shutDown();
	void updateBackground();

};


class SketchPad : public QDialog {
	Q_OBJECT
public:
	SketchPad(QWidget *);
	~SketchPad();
protected:
	void closeEvent(QCloseEvent *);
	void resizeEvent(QResizeEvent *);
private:
	QTabWidget * tabWidget;
	SketchTools * sketchTools;
	SketchPage * sketchPage[SKETCHPAGES];
public slots:
	void newSketch(int player, QByteArray data);
	void clearActualPage();
	void undo();
private slots:
	void tabChanged(int);
};


#endif