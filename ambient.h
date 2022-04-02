#ifndef AMBIENT_H
#define AMBIENT_H
#include <QComboBox>
#include <QFileDialog>
#include <QIcon>
#include <QWidget>
#include <QGroupBox>
#include <QToolButton>
#include <QTimer>
#include <QLabel>
#include "control.h"
#include "config.h"

class Ambient : public QWidget
{
Q_OBJECT
public:
	Ambient(QWidget * parent);
private:
	QGroupBox * frame;
	QToolButton * musicPlayButton, * musicSkipButton, * musicBackButton, * musicShuffleButton, * musicDominantButton;
	QToolButton * soundPlayButton, * soundLoopButton;
	QTimer * timer;
	QComboBox * musicPlayListBox, * soundPlayListBox;
	QToolButton * soundButton;
	QSlider * fadeSlider;
	QIcon * playIcon, * stopIcon, * shuffleOnIcon, * shuffleOffIcon, * dominantOnIcon, * dominantOffIcon;
	QIcon * loopOnIcon, * loopOffIcon;
	bool play;
	QLabel * posLabel;
	QLabel * musicLabel, * soundLabel;
	QString actualSong, actualSongScrolled;
private slots:
	void musicToggle();
	void heartbeat();
	void nextSong();
	void lastSong();
	void changeMusicPlaylist();
	void soundToggle();
	void changeSound();
	void changeFading(int);
	void loopPressed();
	void soundButtonPressed();
public slots:
	void fileUpdate(int, QString);
	
};

#endif