#ifndef PLAYERCOMLINK_H
#define PLAYERCOMLINK_H

#include <QWidget>
#include <QPushButton>
#include <QToolButton>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QLineEdit>
#include <QUdpSocket>
#include <QDialog>
#include <QProcess>
#include <QCheckBox>
#include <assert.h>
#include "config.h"
#include "control.h"


class PlayerComLink : public QWidget 
{
	Q_OBJECT
public:
	enum {NONE, VOICE, MUSIC, PIGGYBACK, DICEROLL, TEXT, NRPACKETTYPES};
	
	PlayerComLink(QWidget * parent = NULL);
	~PlayerComLink();
	void clearSendBuffer(int p);

private:
	QGroupBox * playersFrame; 
	QToolButton * configButton, * newButton, * openButton, * joinButton;
	QLabel * localPlayerNumber, * localPlayerTeam, * localPlayerMute;
	QLabel * remotePlayerNumber[MAXPLAYERLINKS], * remotePlayerMute[MAXPLAYERLINKS];
	QLabel * remotePlayerTeam[MAXPLAYERLINKS], * remotePlayerModified[MAXPLAYERLINKS];
	QLabel * localPlayerName, * localPlayerIcon, * localPlayerRemark;
	QLabel * localEchoLabel;
	QSlider * localEchoVolume;
	QLabel * remotePlayerIcon[MAXPLAYERLINKS], * remotePlayerName[MAXPLAYERLINKS], * remotePlayerAddress[MAXPLAYERLINKS];
	QSlider * remotePlayerVolume[MAXPLAYERLINKS];
	QLabel * remotePlayerStatus[MAXPLAYERLINKS];
	QIcon * gearIcon;
	QTimer * updateTimer, * wgetTimer;
	QDialog * setup;
	QProcess * wget;
	bool complete;
public slots:
	void newPlayerIcon(int player, QByteArray data);
	void update();
private slots:
	void volumeChanged();
	void wgetFinished(int exitCode);
	void localEchoChanged(int state);
	void startWget();

};


#endif