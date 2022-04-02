#ifndef ROOMACOUSTICS_H
#define ROOMACOUSTICS_H

#include <QWidget>
#include <QPushButton>
#include <QToolButton>
#include <QGroupBox>
#include <QComboBox>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "config.h"
#include "control.h"
#include "bulktransfer.h"


class RoomAcoustics : public QWidget 
{
	Q_OBJECT
public: 
	RoomAcoustics(QWidget * parent);
public slots:
	void teamChange();
private:
	QGroupBox * frame; 
	QComboBox * effect;
	QToolButton * playerButton[MAXPLAYERS];
private slots:
signals:
};

#endif