#include <QApplication>
#include <QPushButton>
#include <QTcpSocket>
#include <QDir>
#include <QFileDialog>
#include <QSplashScreen>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "headtracker.h"
#include "mainwindow.h"
#include "config.h"
#include "control.h"
#include "streammanager.h"
#include "painterface.h"
#include "bulktransfer.h"
#include "facilitymanager.h"
#include "tableselector.h"

#include "convert.h"

MainWindow * mainwindow = NULL;
QApplication * app;
TableSelector * tableSelector;
Ctrl * ctrl = NULL; 
FacilityManager * facilitymanager = NULL;
BulkBay * bulkbay = NULL;
StreamManager * streammanager = NULL;
//QSplashScreen * splash;

void quit()
{
	qWarning() << "main; Exiting...";
	if (ctrl != NULL) ctrl->quit();
	if (streammanager != NULL) {
		while (streammanager->isRunning());  // Wait for streammanager to end
		delete streammanager;
	}
	if (mainwindow != NULL) mainwindow->close();
	qDebug("main: Streammanager ended");
	app->quit();
}
  
void tableSelected(void)
{
	//splash->show();
	bulkbay = new BulkBay(); // Bulkbay handles the bulky network traffic
	streammanager = new StreamManager(bulkbay);  // The stream manager does all the "realtime" data mangling in a separate thread - poor guy!
	QObject::connect(streammanager, &StreamManager::finished, streammanager, &QObject::deleteLater);
	facilitymanager = new FacilityManager(); // The facility manager does all the file transfer in the background
	QObject::connect(facilitymanager, &FacilityManager::finished, facilitymanager, &QObject::deleteLater);
	mainwindow = new MainWindow(); 	// GUI is processed in the main thread - according to Qt requirements
	facilitymanager->start(); // Start the facilitymanager process
	mainwindow->show();
	streammanager->start(); // Start the strammanager thread as last action
    QObject::connect(mainwindow, &MainWindow::quit, quit);
	//splash->finish(mainwindow);
}

int main(int argc, char* argv[])
{
    app = new QApplication(argc, argv);
	app->setStyle("Plastique");
	// There is one "global" variable for system control
	ctrl = new Ctrl;
	//splash = new QSplashScreen(QPixmap("icons/ArthursTable.png"));
	if (argc < 2) {
		tableSelector = new TableSelector(mainwindow);
		QObject::connect(tableSelector, &TableSelector::quit, quit);
		QObject::connect(tableSelector, &TableSelector::selected, tableSelected);
	} else {
		ctrl->setTableName(argv[1]);
		tableSelected();
	}
    return app->exec();
}


void closeEvent(QCloseEvent * event)
{
	// Exit gracefully!
	quit();
	event->accept();
}

