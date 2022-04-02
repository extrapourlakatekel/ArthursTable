#ifndef VOICEEFFECT_H
#define VOICEEFFECT_H
#include <QWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QComboBox>
#include <QButtonGroup>
#include <QPushButton>
#include "control.h"
#include "config.h"
#include "soundfx.h"

class VoiceEffect : public QWidget
{
	Q_OBJECT
public:
	VoiceEffect(QWidget * parent);
public slots:
	void functionKeyPressed(int key);
	void functionKeyReleased(int key);
	void voiceEffectChange(int effectNr);
private:
	QGroupBox * frame;
	QLabel * masterLabel, * playerLabel;
	QButtonGroup * selectorGroup;
	QCheckBox * masterEffectSelector[VOICEPRESELECTIONS+1];
	QPushButton * masterEffectButton[VOICEPRESELECTIONS];
	int oldEchoVolume;
	int oldEchoVolume2;
private slots:
	void gamemasterEffectChanged(QAbstractButton * button);
	void effectButtonPressed();
};


#endif