#ifndef TEAMSELECTOR_H
#define TEAMSELECTOR_H

#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
#include <QButtonGroup>
#include <QPushButton>
#include "config.h"
#include "control.h"


class TeamSelector : public QWidget
{
Q_OBJECT
public:
	TeamSelector(QWidget *);
private:
	QGroupBox * frame;
	QPushButton * teamsButton[MAXTEAMS];
	QLabel * masterLabel;
	QLabel * playerNameLabel[MAXPLAYERLINKS];
	QLabel * teamNumberLabel[MAXTEAMS];
	QLabel * hintLabel;
	QCheckBox * teamSelectBox[MAXPLAYERLINKS][MAXTEAMS];
	QButtonGroup * buttonGroup[MAXPLAYERLINKS];
	QButtonGroup * teamGroup;
	QComboBox * effectBox;
	int activeTeam;
private slots:
	void teamButtonPressed(QAbstractButton *);
	void playerButtonPressed(QAbstractButton *);
	void effectChanged(int);

public slots:
	void functionKeyPressed(int);
	void update();
	void remotePlayerNameChange(int);
};


#endif
