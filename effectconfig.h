#ifndef EFFECTCONFIG_H
#define EFFECTCONFIG_H
#include <QDialog>
#include <QCloseEvent>
#include <QLabel>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSlider>
#include <QPushButton>
#include "control.h"

class EffectFrame : public QGroupBox {
	Q_OBJECT
public:
	EffectFrame(int, QWidget *);
	int getRequiredWidth();
	int getRequiredHeight();
protected:
	int requiredWidth, requiredHeight;
	int effectSlot;
	QByteArray config;
private:
};

class EffectNone : public EffectFrame {
	Q_OBJECT
public:
	EffectNone(int, QWidget *);
private:

};

class EffectPitch : public EffectFrame {
	Q_OBJECT
public:
	EffectPitch(int, QWidget *);
private:
	QSlider * pitchSlider;
	QLabel * leftLabel, * centerLabel, * rightLabel;
public slots:
	void sliderMoved(int);
};

class EffectSwarm : public EffectFrame {
	Q_OBJECT
public:
	EffectSwarm(int, QWidget *);
private:
	QSlider * voicesSlider;
	QLabel * voicesTitleLabel, * voicesLeftLabel, *voicesCenterLabel, * voicesRightLabel;
	QSlider * pitchSlider;
	QLabel * pitchTitleLabel, * pitchLeftLabel, * pitchCenterLabel, * pitchRightLabel;
	QSlider * pitchVariationSlider;
	QLabel * pitchVariationTitleLabel, * pitchVariationLeftLabel, * pitchVariationCenterLabel, * pitchVariationRightLabel;
	QSlider * delayVariationSlider;
	QLabel * delayVariationTitleLabel, * delayVariationLeftLabel, * delayVariationCenterLabel, * delayVariationRightLabel;
	QSlider * volumeVariationSlider;
	QLabel * volumeVariationTitleLabel, * volumeVariationLeftLabel, * volumeVariationCenterLabel, * volumeVariationRightLabel;
	QPushButton * rerollButton;
private slots:
	void settingsChanged();
	void reroll();
};

class EffectConfig : public QDialog {
	Q_OBJECT
public:
	EffectConfig(QWidget *);
public slots:	
	void configureEffect(int);
protected:
	void closeEvent(QCloseEvent *);
private:
	QLabel * muteHint;
	EffectFrame * effectFrame;
	QComboBox * effectEngineBox;
	QLineEdit * effectName;
	QPushButton * doneButton;
	bool wasMuted;
	int oldEchoVolume;
	int oldSlot;
	int actualEngine;
	int actualSlot;
private slots:
	void nameChanged(QString);
	void showEffectEngine(int);
	void doneButtonPressed();
};

#endif