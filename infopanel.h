#ifndef INFOPANE_H
#define INFOPANE_H

#include <QWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QTextEdit>
#include "config.h"
#include "control.h"
#include "mapviewer.h"
#include "npcviewer.h"
#include "dungeonviewer.h"
#include "bulktransfer.h"
#include "viewer.h"

class InfoPanel : public QWidget 
{
	Q_OBJECT
public: 
	InfoPanel(QWidget * parent = NULL);
	~InfoPanel();
	void setSize(int width, int height);
	bool isFullScreen();
	void forwardKeyPressEvent(QEvent *);
protected:
	void resizeEvent(QResizeEvent * event);
	void keyPressEvent(QKeyEvent *);
	void keyReleaseEvent(QKeyEvent *);
private:
	void setSize();
	void visibilityChange();
	QWidget * parent;
	QToolButton * fullscreenButton;
	QTabWidget * tabWidget;
	NpcViewer * npcViewer;
	QVector<Viewer *> viewer;
	QTextEdit * textOutput;
	QTextEdit * textInput;
	bool isFullscreen;
	int w,h;
	int oldteam;
private slots:
	void toggleFullscreen();
	void textEntered();
public slots:
	void newChatMessage(int player, QByteArray data);
	void escKeyPressed();
	void displayText(QString text, int format, bool bot);
	void focus();
	void newViewerEvent(int, QByteArray);
	void fileUpdate(int dir, QString filename);

};

#endif

