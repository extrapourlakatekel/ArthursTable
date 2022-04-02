#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QResizeEvent>
#include <QStackedLayout>
#include <QTimer>
#include <QPushButton>
#include <QInputEvent>
#include <QFileDialog>
#include "ambient.h"
//#include "ambientsound.h"
#include "ambiani.h"
#include "audioio.h"
#include "bulktransfer.h"
#include "chatwindow.h"
#include "commatrix.h"
#include "control.h"
#include "diceroller.h"
#include "effectconfig.h"
#include "filewatcher.h"
#include "headtracker.h"
#include "infopanel.h"
#include "messages.h"
#include "painterface.h"
#include "playercomlink.h"
#include "roomacoustics.h"
#include "setup.h"
#include "sketchpad.h"
#include "teamselector.h"
#include "voiceeffect.h"
#include "visualizer.h"



class MainWindow : public QWidget {
	Q_OBJECT
public:
	MainWindow(QWidget * parent = NULL);
	~MainWindow();
public slots:
	void layoutChange();
protected:
	void closeEvent(QCloseEvent * event);
	void resizeEvent(QResizeEvent * event);
	void keyPressEvent(QKeyEvent *);
	void keyReleaseEvent(QKeyEvent *);
private:
	SketchPad * sketchPad;
	EffectConfig * effectConfig;
	FileWatcher * fileWatcher;
	QWidget * innerWidget;
	QVBoxLayout * mainLayout;
	QStackedLayout * stackedLayout;
	Headtracker * headtracker;
	AudioIo * audioIo;
	PlayerComLink * playerComLink;
	DiceRoller * dice;
	InfoPanel * infoPanel;
	QPushButton * setupButton, * reloadButton, * visualizerButton, *sketchpadButton;
	QPushButton * filetransferButton;
	QWidget * setupWidget;
	Setup * setup;
	Visualizer * visualizer = NULL;
	Ambient * ambient;
	//AmbientSound * ambientSound;
	VoiceEffect * voiceEffect;
	RoomAcoustics * roomAcoustics;
	TeamSelector * teamSelector;
	Messages * messages;
	int16_t * inframe, * outframe;
	bool visualizerActive = false;
	int lastkeyforwarded;
private slots:
	void startSetup();
	void startVisualizer();
	void visualizerEnded();
	void showSketchpad();
	void openFileDialog();
	void reloadButtonPressed();
signals:
	void quit();
};


#endif