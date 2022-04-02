#ifndef AUDIOIO_H
#define AUDIOIO_H

#include <QWidget>
#include <QPushButton>
#include <QToolButton>
#include <QGroupBox>
#include <QSlider>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <assert.h>
#include <math.h>
#include <QCheckBox>
#include "vumeter.h"
#include "control.h"

#define MONO 1
#define STEREO 2
#define BYTESPERSAMPLE 2

class AudioIo : public QWidget 
{
	Q_OBJECT
public: 
	AudioIo(QWidget * parent);
	float * inframe;
	float * outframe;
public slots:
	void localPlayerNameChange();
	void functionKeyPressed(int key);
	void functionKeyReleased(int key);
	void teamOrEffectChange();
	void layoutChange();
	void voiceEffectChange();
private:
	bool initAudio(char *id, char *od, uint32_t fs, uint32_t samplerate);
	bool stopAudio();
	QToolButton * muteButton;
	QGroupBox * audioInFrame, * ambientFrame, * audioOutFrame; 
	//QToolButton * configInButton, * configOutButton, * configAmbientButton;
	VuMeter * inputLevel, * outputLevelLeft, * outputLevelRight;
	QSlider * inputSensitivity, * ambientVolume;
	QCheckBox * voiceEffectBox;
	QToolButton * voiceEffectButton;
	QCheckBox * commEffectBox;
	int framecount;
	int framewidth;
	int fs;
	bool verbose = false;
	float peakin, peakoutleft, peakoutright;
	int16_t *buf;
    pa_simple *rec = NULL;
    pa_simple *play = NULL;
	QTimer * timer;
	QIcon * mikeIcon, * muteIcon;
	bool talktootherteams;
private slots:
	void updateVu();
	void setInputSensitivity(int sensitivity);
	void setAmbientVolume(int volume);
	void toggleMute();
	void voiceEffectStateChange(int state);
	void commEffectStateChange(int state);
	void voiceEffectButtonPressed();
};

#endif