#ifndef FACILITYMANAGER_H
#define FACILITYMANAGER_H
#include <QThread>
#include <QFile>
#include <QPixmap>
#include <QTimer>
#include <QStringList>
#include <QDir>
#include "config.h"
#include "control.h"
#include "bulktransfer.h"
#include "streammanager.h"
#include "types.h"

QByteArray pack(QVector <FileInfo> in);
void unpack(QVector <FileInfo> &out, QByteArray in);
QByteArray pack(WholeFile in);
void unpack(WholeFile &out, QByteArray in);

class FacilityManager : public QThread
{
	Q_OBJECT;
public:
	FacilityManager();
	void run() override; // Shall be protected, only for testing purposes
protected:
public slots:
	void resetConnection(int player);
	void newFileInfo(int player, QByteArray data);
	void newFileTransfer(int player, QByteArray data);
	void addFileToSend(FileInfo file);
private:
	int waitcount[MAXPLAYERLINKS];
	QTimer * timer;
	QVector <FileInfo> localfiles;
	QVector <FileInfo> remotefiles[MAXPLAYERLINKS];
	QVector <FileInfo> filestosend[MAXPLAYERLINKS];
	bool sendIcon(int player);
	void scanForFiles();
	bool iconSent[MAXPLAYERLINKS], filesReported[MAXPLAYERLINKS], briefingDone[MAXPLAYERLINKS];
private slots:
	void timerTick();
};

extern FacilityManager * facilitymanager;

#endif