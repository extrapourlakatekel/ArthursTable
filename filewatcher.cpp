#include "filewatcher.h"


FileWatcher::FileWatcher()
{
	watchedfiles = scan();
}

void FileWatcher::rescan()
{
	QVector<WatchedFile> newfiles;
	newfiles = scan();
	int i,k;
	for (i=0; i<newfiles.size(); i++) {
		for (k=0; k<watchedfiles.size(); k++) {
			if (newfiles.at(i).dir == watchedfiles.at(k).dir && newfiles.at(i).name == watchedfiles.at(k).name && 
				newfiles.at(i).modified == watchedfiles.at(k).modified) break; // Found the same file
		}
		if (k == watchedfiles.size()) {
			qDebug() << "FileWatcher:" << newfiles.at(i).name << "in directory" << ctrl->dir[newfiles.at(i).dir] << "is new";
			emit(ctrl->fileUpdate(newfiles.at(i).dir, newfiles.at(i).name));
		}
	}
}

QVector<WatchedFile> FileWatcher::scan()
{
	QVector<WatchedFile> f;
	QStringList filters;
	WatchedFile wf;
	filters << "*"; 
	for (int d=0; d<NDIRS; d++) {
		qDebug() << "FileWatcher: Scannig directory" << ctrl->dir[d].path() << "for files and direcories";
		QStringList filesindirectory;
		if (d == DIR_MUSIC) filesindirectory = ctrl->dir[d].entryList(filters, QDir::Dirs | QDir::NoDotAndDotDot); //For music only new directories are important
		else filesindirectory = ctrl->dir[d].entryList(filters, QDir::Files);
		for (int i=0; i<filesindirectory.size(); i++) {
			wf.dir = d;
			wf.name = filesindirectory.at(i);
			QFileInfo info(ctrl->dir[d].filePath(filesindirectory.at(i)));
			wf.modified = info.lastModified();
			qDebug() << "    Found file/directory:" << wf.name <<"with modification date" << wf.modified;
			f << wf;
		}
	}
	return f;
}
