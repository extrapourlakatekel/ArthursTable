#ifndef AMBIENTMUSIC_H
#define AMBIENTMUSIC_H
#include <QWidget>
#include <QGroupBox>
#include <QToolButton>
#include <QIcon>
#include <QComboBox>
#include <QTimer>
#include <QLabel>
#include "control.h"
#include "config.h"

class AmbientMusic : public QWidget
{
Q_OBJECT
public:
	AmbientMusic(QWidget * parent);
private:
	QGroupBox * frame;
	QToolButton * playButton, * skipButton, * backButton, * shuffleButton, * dominantButton;
	QTimer * timer;
	QComboBox * playListBox;
	QIcon * playIcon, * stopIcon, * shuffleOnIcon, * shuffleOffIcon, * dominantOnIcon, * dominantOffIcon;
	bool play;
	QLabel * posLabel;
	QString actualSong, actualSongScrolled;
private slots:
	void toggle();
	void heartbeat();
	void nextSong();
	void lastSong();
	void changePlaylist();
public slots:
	void fileUpdate(int, QString);
	
};


#endif