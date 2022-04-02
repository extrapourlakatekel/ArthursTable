#ifndef VIEWER_H
#define VIEWER_H
#include <inttypes.h>
#include <malloc.h>
#include <QAction>
#include <QActionGroup>
#include <QContextMenuEvent>
#include <QDebug>
#include <QGuiApplication>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QToolButton>
#include <QResizeEvent>
#include <QWidget>
#include <QWidgetAction>
#include <wchar.h>
#include "algorithms.h"
#include "bulktransfer.h"
#include "control.h"
#include "config.h"
#include "marker.h"
#include "tools.h"

#ifdef QT_NO_CONTEXTMENU
#error "Sorry - Qt library with context menu support is required!"
#endif

// Generic viewer for picture based things like maps, dungeons and handouts

#define RADII 11

#define MINBRIGHT 30

enum ViewMode {VIEW_HIDDEN, VIEW_NORMAL, VIEW_FOG, VIEW_DARKNESS, VIEW_ACCESSIBLE, VIEW_VISIBLE, VIEW_VISIBLERADAR, VIEW_VISIBLEBLUEPRINT, VIEWMODES};

class Viewer : public QWidget
{
	enum Commands {CLEARALL, SETZOOM, SETCENTER, SETRIGHTS, SETMARKER, SETVIEWMODE, FOCUS, SCALETOFIT};
	Q_OBJECT;
public:
	Viewer(QString, QWidget * = NULL);
	QString getName();
	QString getFilename();
	bool isVisible();
	bool setConfig(QByteArray *);
protected:
	void paintEvent(QPaintEvent * event) override;
	void mousePressEvent(QMouseEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);
	void mouseMoveEvent(QMouseEvent * event);
	void wheelEvent(QWheelEvent * event);
	void resizeEvent(QResizeEvent * event);
    void contextMenuEvent(QContextMenuEvent *event) override;
private:
	QByteArray getConfig();
	void render();
	void renderViewHidden(int);
	void renderViewNormal(int);
	void renderViewRadius(int, QColor, int);
	void renderViewAccessible(int, int);
	void renderViewVisible(int, int);
	void renderViewVisibleWithRadar(int, int);
	void renderViewVisibleWithBlueprint(int, int);
	void loadImage();
	void loadConfig();
	void saveConfig();
	void clearConfig();
	void adjustMenu();
	void broadcast(QByteArray);
	void sendRights();
	QString filename, myname;
	QByteArray buffer[MAXPLAYERLINKS];
	QTimer timer;
	QImage map;
	bool * wall;
	uint16_t * overlay;
	QImage art;
	float zoom[MAXTEAMS];
	float zoomxcenter[MAXTEAMS], zoomycenter[MAXTEAMS];
	QPoint dragstartpos;
	bool allowScrolling, allowZooming, showOtherTeams, showTeamMarkers;
	// Menus
	QAction * focusAct;
	QAction * zoomToFitAct;
	QAction * allowScrollingAct;
	QAction * allowZoomingAct;
	// Everything for the marker menu
	QAction * showTeamMarkerAct;
	QAction * showOtherTeamsAct;
	QAction * newMarkerAct;
	
	// Everything for the view menu
	uint8_t        viewmode[MAXTEAMS];
	QMenu *        viewmodeMenu[MAXTEAMS];
	QActionGroup * viewmodeGroup[MAXTEAMS];
	QAction *      viewmodeAct[MAXTEAMS][VIEWMODES];
	QString        viewmodeName[VIEWMODES] = {"Hide Image", "Normal", "Fog", "Darkness", "Accessible Area", "Visible Area", "Visible Over Radar", "Visible Over Plan"};
	
	//QAction * viewRadiusAct[MAXTEAMS];
	// Everything for the radius menu
	uint8_t        radiusStep[MAXTEAMS]; // To make it clear, this variable keeps the radius STEP not the radius itself
	QMenu *        radiusMenu[MAXTEAMS];
	QActionGroup * radiusGroup[MAXTEAMS];
	QAction *      radiusAct[MAXTEAMS][RADII];
	int            radiusSize[RADII] = {25, 40, 63, 100, 160, 250, 400, 630, 1000, 1600, 2500};

	// Markers
	QMenu * markerMenu;
	QAction * removeMarkersAct;
	MarkerSelection * markerSelection;

	int menuatx, menuaty; // The focus needs to know where the menu had been opened
	Marker * marker[MAXMARKERS];
public slots:
	void fileUpdate(int, QString);
	void teamChange();
	void gamemasterChange(); 
	void connectionStateChange(int);
	void shutDown();
protected slots:
	void markerDragged(uint8_t);
	void markerDeleted(uint8_t);
private slots:
	void transmit();
	void radiusSet(QAction *);
	void viewmodeSet(QAction *);
	void newMarker(int style, int colorindex);
	void allowScrollingChanged();
	void allowZoomingChanged();
	void showTeamMarkersChanged();
	void showOtherTeamsChanged();
	void focusRequested(bool);
	void removeMarkers(bool);
	void zoomToFitRequested(bool);
signals:
	void focusMe();
};

#endif