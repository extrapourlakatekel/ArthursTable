#ifndef VISUALIZER_H
#define VISUALIZER_H
#include <math.h>
#include <QDialog>
#include <QGuiApplication>
#include <QLabel>
#include <QPaintEvent>
#include <QScrollArea>
#include <QStringList>
#include <QTimer>
#include <QWheelEvent>
#include <QPainter>
//using namespace QtCharts;
#include "bulktransfer.h"
#include "control.h"

#define PLOTWIDTH 100

class StatusIndicator : public QWidget
{
	Q_OBJECT
public:
	StatusIndicator(QWidget *);
	void set(Status, float);
private:
	QLabel * statuslabel, * fillinglabel;
	Status oldstatus;
	float oldfilling;
};

class Plot : public QWidget
{
	Q_OBJECT
public:
	Plot(QWidget *);
	void addData(int rxpackets, int txbytes, int rxbytes);
protected:
	void paintEvent(QPaintEvent *event) override;
private:
	int * rxp, * txb, * rxb;	
	int pos;
	//QLineSeries * series;
	//QChart * chart;
	//QChartView * chartView;	
};


class Visualizer : public QDialog {
	Q_OBJECT
public:
	Visualizer(QWidget *);
	~Visualizer();
private:
	QTimer * timer;
	StatusIndicator * txindicator[MAXPLAYERLINKS][MAXBULKBAYS];
	StatusIndicator * rxindicator[MAXPLAYERLINKS][MAXBULKBAYS];
	Plot * plot[MAXPLAYERLINKS];
	QLabel * rxlabel, * txlabel;
	QLabel *playerlabel[MAXPLAYERLINKS];
private slots:
	void update();
	void statistics(QList<int>, QList<int>, QList<int>);
};


#endif