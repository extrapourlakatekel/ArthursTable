#include <assert.h>
#include "facilitymanager.h"

QByteArray pack(QVector <FileInfo> in)
{
	QByteArray out("");
	for(int i=0; i<in.size(); i++) {
		out.append((char)in.at(i).dirnr); // Number of the directory
		assert (in.at(i).name.size() < 256);
		out.append((char)(uint8_t)in.at(i).name.toUtf8().size()); // Size of the filename
		out.append(in.at(i).name.toUtf8()); // Filename
		assert (in.at(i).hash.size() > 0);
		assert (in.at(i).hash.size() < 256);
		out.append((char)(uint8_t)in.at(i).hash.size()); // Size of the hash
		out.append(in.at(i).hash); // The hash itself
	}
	return out;
}

void unpack(QVector <FileInfo> &out, QByteArray in)
{
	FileInfo f;
	int fns, hs;
	out.clear();
	while (1) {
		// FIXME: Update this to fromNBO()!!!
		if (in.size() < 2) {qWarning("unpack <FileInfo>: Missing first two bytes"); return;}
		f.dirnr = (uint8_t) in.at(0);
		fns = (uint8_t) in.at(1);
		if (in.size() < 3 + fns) {qWarning("unpack <FileInfo>: Missing bytes for filename or hash length"); return;}
		f.name = in.mid(2, fns);
		assert(fns+2 < in.size());
		hs = (uint8_t) in.at(fns+2);
		if (in.size() < 3 + fns + hs) {qWarning("unpack <FileInfo>: Missing bytes for hash"); return;}
		f.hash = in.mid(fns + 3, hs);
		//qDebug() << "unpack: extracted" << f.name << "and its hash" << f.hash.toHex();
		in.remove(0, fns + hs + 3);
		out << f;
		if (in.size() == 0) return;
	}
}


QByteArray pack(WholeFile in) 
{
	QByteArray out("");
	out.append(in.dirnr); // Number of the directory where the file is to place
	int fns = in.name.toUtf8().size();
	assert(fns < 256); // Such a long name for a filename without path? Trying to fool me?
	out.append((uint8_t)fns); // File name size
	out.append(in.name.toUtf8());
	out.append(in.data);
	return out;
}

void unpack(WholeFile &out, QByteArray in) 
{
	if (in.size() < 2) {qWarning("unpack <WholeFile>: Missing first two bytes"); return;}
	out.dirnr = in.at(0);
	int fns = (uint8_t) in.at(1);
	if (in.size() < 2 + fns) {qWarning("unpack <WholeFile>: Missing data for filename"); return;}
	out.name = in.mid(2, fns);
	out.data = in.mid(2 + fns); // The rest of the QByteArray has to be the data - no need to know the specific size
	
}

FacilityManager::FacilityManager() : QThread()
{
	this->setObjectName("gotongi facility manager thread");
}

void FacilityManager::run()
{
	scanForFiles();
	qDebug("FacilityManager: Scan for files done!");
	// Tap into all events thay may concern us
	timer = new(QTimer);
	timer->setInterval(200);
	timer->setSingleShot(false);
	timer->start();
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		waitcount[p] = 0; // FIXME: What's that for?
		iconSent[p] = false;
		filesReported[p] = false;
	}
	
	connect(ctrl,          SIGNAL(resetConnection(int)),             this, SLOT(resetConnection(int)));
	connect(bulkbay,       SIGNAL(newFileInfo(int, QByteArray)),     this, SLOT(newFileInfo(int, QByteArray)));
	connect(bulkbay,       SIGNAL(newFileTransfer(int, QByteArray)), this, SLOT(newFileTransfer(int, QByteArray)));
	connect(timer,         SIGNAL(timeout()),                        this, SLOT(timerTick()));
	// Start the Qt event system - everything else is done event based
	ctrl->facilityManagerReady.set(true);
	exec();
}

void FacilityManager::resetConnection(int player)
{
	assert(player<MAXPLAYERLINKS);
	qDebug("FacilityManager: Connectionn of player %d reset", player);
	waitcount[player] = 0;
	iconSent[player] = false;
	filesReported[player] = false;
	briefingDone[player] = false;
}

void FacilityManager::newFileInfo(int player, QByteArray data)
{
	assert(player < MAXPLAYERLINKS);
	qDebug("FacilityManager: Got file info from player %d", player);
	if (ctrl->mode.get() != MODE_MASTER) {qWarning("Facility Manager: Got file info while not beeing gamemaster - ignoring it. Are you messing with network addresses?"); return;}
	unpack(remotefiles[player], data);
	filestosend[player].clear();
	// Now check which files are missing on the remote host
	for (int l=0; l<localfiles.size(); l++) {
		qDebug() << "FacilityManager: Check whether player needs" << localfiles.at(l).name;
		bool found = false;
		for (int r=0; r<remotefiles[player].size(); r++) {
			if (localfiles.at(l) == remotefiles[player].at(r)) {found = true; break;}
		}
		if (!found) {
			qDebug() << "FacilityManager: Player" << player << "needs file" << localfiles.at(l).name;
			filestosend[player] << localfiles.at(l);
		} else qDebug("    no.");
	}
}

void FacilityManager::newFileTransfer(int player, QByteArray data)
{
	qDebug("FacilityManager: Got file transfer from player %d", player);
	WholeFile wholefile;
	unpack(wholefile, data);
	QFile file(ctrl->dir[wholefile.dirnr].filePath(wholefile.name));
	if (!file.open(QIODevice::WriteOnly)) {qWarning("FacilityManager: Cannot write received file to disk!");}
	else file.write(wholefile.data);
	file.close();
	// We got a new file, so add it to the list of available files. Otherwise the master 
	// starts to resend all the files when restarted. But calculate the hash in case a
	// transmission error had occured so it could be resent when trying again.
	QCryptographicHash hash(QCryptographicHash::Md5);
	FileInfo fileondisk;
	fileondisk.dirnr = wholefile.dirnr;
	fileondisk.name = wholefile.name;
	hash.addData(wholefile.data);
	fileondisk.hash = hash.result();
	// We got this file from the master so we assume it is new - so don't check whether it already exists
	localfiles << fileondisk;
	qDebug("FacilityManager: Emitting file receiption signal for directory %d", wholefile.dirnr);
	emit(ctrl->fileUpdate((int)wholefile.dirnr, wholefile.name));
}

bool FacilityManager::sendIcon(int player)
{
	QPixmap playerIcon;
	QByteArray data;
	QBuffer buffer(&data);
	if (bulkbay->getBufferUsage(player) > BUF_LOW_PRIO) return false; // When the buffer is already that full, leave it to more important tasks
	if (ctrl->mode.get() == MODE_MASTER) playerIcon.load("icons/gamemaster.png");
	else playerIcon.load(ctrl->dir[DIR_WORKING].filePath("playericon.png"));
	if (playerIcon.isNull()) return true; // Player does not have an icon but proceed to next status anyway
	buffer.open(QIODevice::WriteOnly);
	playerIcon.scaled(60, 80, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).save(&buffer, "PNG");
	return bulkbay->addToTxBuffer(player, PLAYERICON, data) >= 0;
}

void FacilityManager::scanForFiles()
{
	QStringList filters;
	bool foundnewone = false;
	filters << "*.png" << "*.PNG" << "*.jpg" << "*.JPG" << "*.cfg"; // Do we really have to limit the types?
	// Scan all relevant dirs
	for (int d=0; d<ctrl->dirstosend.size(); d++) {
		qDebug() << "FacilityManager: Scannig directory for files" << ctrl->dir[ctrl->dirstosend.at(d)].path();
		QStringList filesindirectory = ctrl->dir[ctrl->dirstosend.at(d)].entryList(filters, QDir::Files);
		for (int f=0; f<filesindirectory.size(); f++) {
			FileInfo fileondisk;
			QCryptographicHash hash(QCryptographicHash::Md5);
			QFile file(ctrl->dir[ctrl->dirstosend.at(d)].filePath(filesindirectory.at(f)));
			if (!file.open(QIODevice::ReadOnly)) {qWarning() << "FacilityManager: Could not open file" << filesindirectory.at(f) << "for hashing";}
			else {
				int i;
				fileondisk.dirnr = ctrl->dirstosend.at(d);
				fileondisk.name = filesindirectory.at(f);
				hash.addData(&file);
				fileondisk.hash = hash.result();
				qDebug() << "FacilityManager: Found file" << fileondisk.name << "with hash" << hash.result().toHex();
				for (i=0; i<localfiles.size(); i++) if (localfiles.at(i) == fileondisk) break;
				if (i == localfiles.size()) {
					foundnewone = true;
					localfiles << fileondisk;
				}
				file.close();
			}
		}
	}
}


void FacilityManager::timerTick()
{
	int mode = ctrl->mode.get();
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		
		if (ctrl->connectionStatus.get(p) == CON_READY) {

			if (!iconSent && bulkbay->getBufferUsage(p) <= BUF_LOW_PRIO) if (sendIcon(p)) iconSent[p] = true;

			if (mode != MODE_MASTER && !filesReported[p])  {
				if (bulkbay->getBufferUsage(p) <= BUF_LOW_PRIO && bulkbay->addToTxBuffer(p, FILEINFO, pack(localfiles)) >= 0 ) {
					qDebug() << "FacilityManager: Informed master about the local files:";
					for (int i=0; i<localfiles.size(); i++) qDebug() << localfiles[i].name;
					filesReported[p] = true;
				}
			}
			if (mode == MODE_MASTER && !briefingDone[p]) {
				if (bulkbay->getBufferUsage(p) <= BUF_HIGH_PRIO) if (ctrl->briefRemotePlayers()) briefingDone[p] = true; // FIXME: Move this to the control section
			}
			if (!filestosend[p].isEmpty() && bulkbay->getBufferUsage(p) < BUF_LOW_PRIO) {
			
				WholeFile wholefile;
				assert (filestosend[p].last().dirnr < NDIRS);
				QFile file(ctrl->dir[filestosend[p].last().dirnr].filePath(filestosend[p].last().name));
				qDebug() << "Sending file" << ctrl->dir[filestosend[p].last().dirnr].filePath(filestosend[p].last().name) << "to player" << p;
				//qDebug() << "FacilityManager" << filestosend[p].last().dirnr;
				//qDebug() << "FacilityManager:" << ctrl->dir[filestosend[p].last().dirnr];

				if (!file.open(QIODevice::ReadOnly)) {
					qDebug() << "FacilityManager: Cannot open file to send, skipping" << filestosend[p].last().name;
					filestosend[p].removeLast(); 
				}
				else {
					wholefile.data = file.readAll();
					wholefile.dirnr = filestosend[p].last().dirnr;
					wholefile.name = filestosend[p].last().name;
					int filetransferindex = bulkbay->addToTxBuffer(p, FILETRANSFER, pack(wholefile), true, false); // report but do all transfers in parallel
					if (filetransferindex < 0) qWarning("FacilityManager: Could not add to bulkbay, error: %d", filetransferindex);
					else {
						remotefiles[p] << filestosend[p].last(); // Remote player will soon have that file
						filestosend[p].removeLast(); // When file is on the way we can remove it from the list
						emit(ctrl->fileTransferStart(filetransferindex, wholefile.name, p));
					}
				}
			}
		}
	}
}

// FIXME: Was ist das denn?
void FacilityManager::addFileToSend(FileInfo file)
{
	// If we're gamemaster then send file to all, if we're player send the file to the gamemaster only
	if (ctrl->mode.get() == MODE_MASTER) {
		for (int p=0; p<MAXPLAYERLINKS; p++) if (ctrl->remotePlayerName.get(p) != "") filestosend[p] << file;
	} else {
		for (int p=0; p<MAXPLAYERLINKS; p++) if (ctrl->remotePlayerName.get(p) == "GAMEMASTER") filestosend[p] << file;
	}
}