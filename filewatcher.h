#ifndef FILEWATCHER_H
#define FILEWATCHER_H
#include <QDateTime>
#include <QDir>
#include <QFile>
#include "control.h"

typedef struct {
	int dir;
	QString name;
	QDateTime modified;
} WatchedFile;

class FileWatcher : QObject {
	Q_OBJECT
public:
	FileWatcher();
	void rescan();
private:
	QVector<WatchedFile> scan();
	QVector<WatchedFile> watchedfiles;
};


#endif