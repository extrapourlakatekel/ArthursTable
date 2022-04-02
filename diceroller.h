#ifndef DICE_H
#define DICE_H

#include <QWidget>
#include <QPushButton>
#include <QToolButton>
#include <QGroupBox>
#include <QComboBox>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "vumeter.h"
#include "config.h"
#include "control.h"
#include "bulktransfer.h"
#include "types.h"


class DiceRoller : public QWidget 
{
	Q_OBJECT
public: 
	DiceRoller(QWidget * parent);
public slots:
	void chatCommand(QString c);
	void newDiceRoll(int, QByteArray);
private:
	QGroupBox * frame; 
	QPushButton * rollButton;
	QIcon * diceIcon;
	QComboBox * ammountBox, * diceTypeBox;
private slots:
	void roll();
signals:
	void result(int, QByteArray);
};

#endif