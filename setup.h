#ifndef SETUP_H
#define SETUP_H
#include <QDialog>
#include <QPushButton>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include "headtracker.h"

#include "control.h"

// FIXME: Probably move the sub-setups to the corresponding modules
class AudioSetup : public QGroupBox
{
	Q_OBJECT
public:
	AudioSetup(int width, QWidget * parent = NULL);
	void store();
private:
	QSpinBox * paRelaxSpinBox;
	QLabel * paRelaxLabel;
};

class TrackerSetup : public QGroupBox
{
	Q_OBJECT
public:
	TrackerSetup(int width, QWidget * parent = NULL);
	void store();
private:
	QLabel * trackerLabel, * deviceLabel;
	QComboBox * trackerBox, * deviceBox;
	bool changes = false;
private slots:
	void listDevices(int);
signals:
	void initTracker();
};

class NetworkSetup : public QGroupBox
{
	Q_OBJECT
public:
	NetworkSetup(int width, QWidget * parent = NULL);
};

class Setup : public QDialog
{
	Q_OBJECT
public:
	Setup(Headtracker * tracker, QWidget * parent = NULL);
private:
	AudioSetup * audioSetup;
	TrackerSetup * trackerSetup;
	NetworkSetup * networkSetup;
	
	QGroupBox * comGroupBox;
	QLabel * tableName;
	QLineEdit * playerName, * serverName, * passphrase;
	QLineEdit * remotePlayersName[MAXPLAYERLINKS];
	QLineEdit * remotePlayersIP[MAXPLAYERLINKS];
	QLineEdit * remotePlayersPort[MAXPLAYERLINKS];
	QCheckBox * useServerCheck;
	QPushButton * doneButton, * cancelButton;
	void get();
	
private slots:
	void done();
	void cancel();
	void useServer(bool state);

signals:
	void redraw();
};

#endif

