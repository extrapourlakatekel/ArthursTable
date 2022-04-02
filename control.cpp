#include "control.h"
#include <assert.h>
#include <string.h>

void broadcast(QByteArray data)
// Wrapper required because the template implementation has to be defined in the .h file which does not know our global bulkbay variable
{
	qDebug("Ctrl: broadcasting remotecontrol");
	// Check whether we're gamemaster - if not, do not broadcast
	if (ctrl->localPlayerName.get() != "GAMEMASTER") return;
	bulkbay->broadcast(REMOTECONTROL, data);
}

void sendout(int player, QByteArray data)
{
	// Check whether we're gamemaster - if not, do not broadcast
	if (ctrl->localPlayerName.get() != "GAMEMASTER") return;
	bulkbay->addToTxBuffer(player, REMOTECONTROL, data);
}


Ctrl::Ctrl() : QObject()
{
	run();
	basedir = QDir(QDir::homePath()+"/"+BASEDIR);
	if (!basedir.exists()) {
		qDebug("Ctrl: Basedirectory does not exist - creating it.");
		if (!basedir.mkpath(".")) qFatal("Ctrl: Directory cannot be created!");
	}
	//sfxPlayer = new QProcess(this);
	// The connection to bulkbay is done in bulkbtransfer.cpp because it is not ready yet, since Ctrl is initialized earlier
}

QDir Ctrl::createDir(QString name)
{
	QDir dir(name);
	if (!dir.exists()) {
		qDebug() << "Ctrl: Directory" << name << "does not exist - creating it.";
		if (!dir.mkpath(".")) qFatal("Ctrl: Directory cannot be created!");
	}
	return dir;
}

void Ctrl::setTableName(QString name)
{
	tablename.set(name);
	dirstosend.clear(); // Should not be required, but if anyone calls setTableName twice...
	dir[DIR_WORKING]  = createDir(basedir.path() + "/" + name);
	dir[DIR_NPCS]  = createDir(dir[DIR_WORKING].path() + "/npcs");
	dirstosend << DIR_NPCS;
	dir[DIR_PICS]     = createDir(dir[DIR_WORKING].path() + "/pics");
	dirstosend << DIR_PICS;
	dir[DIR_CONFIG] = createDir(dir[DIR_WORKING].path() + "/config");
	dirstosend << DIR_CONFIG;
	// These will be streamed and will stay empty on player side
	dir[DIR_MUSIC]    = createDir(dir[DIR_WORKING].path() + "/music");
	dir[DIR_SOUND]      = createDir(dir[DIR_WORKING].path() + "/sound");
	dir[DIR_SHARE] 	  = createDir(dir[DIR_WORKING].path() + "/share");
	dir[DIR_SKETCHES] = createDir(dir[DIR_WORKING].path() + "/sketches");
	
	QFile * file;
	file = new QFile(dir[DIR_WORKING].filePath("settings.cfg"));
	if (!file->open(QIODevice::ReadOnly)) {
		qDebug("Ctrl: Settings do not exist - starting with default values.");
	} else {
		punchingServer.load(file);
		useServer.load(file);
		remotePlayerName.load(file);
		remotePlayerAddress.load(file);
		remotePlayerVolume.load(file);
		localPlayerName.load(file);
		if (localPlayerName.get() == "GAMEMASTER") mode.set(MODE_MASTER); else mode.set(MODE_PLAYER);
		passPhrase.load(file);
		packetHeader.set(generatePacketHeader(passPhrase.get()));
		//broadcastVoice.load(file);
		broadcastAmbient.load(file);
		localEchoVolume.load(file);
		inputSensitivity.load(file);
		ambientVolume.load(file);	
		
		ambientFade.load(file);
		musicPos.load(file);
		soundFile.load(file);
		soundLoop.load(file);
		//ambientMusicShuffle.load(file);
		ambientVolume.load(file);
		paRelaxation.load(file);
		team.load(file);
		//comEffect.load(file); // Do not store the com effect. Players may be mute at startup
		// Load some remotley controlled settings as well - in case we might be master
		npcVisibility.load(file);
		headTrackerType.load(file);
		headTrackerDevice.load(file);
		voiceEffectConfig.load(file);
		voiceEffectName.load(file);
		activeTab.load(file);
		activeSketchTab.load(file);
		qDebug("Ctrl: Config file read");
		//emit(redraw()); // Changes may affect GUI
	}
}

void Ctrl::quit(void)
{
	QSaveFile * file;
	file = new QSaveFile(dir[DIR_WORKING].filePath("settings.cfg"));
	if (!file->open(QIODevice::WriteOnly)) {
		qWarning("Ctrl: Cannot write settings to disk!");
	} else {
		// Store persistent settings
		punchingServer.store(file);
		useServer.store(file);
		remotePlayerName.store(file);
		remotePlayerAddress.store(file);
		remotePlayerVolume.store(file);
		localPlayerName.store(file);
		passPhrase.store(file);
		//broadcastVoice.store(file);
		broadcastAmbient.store(file);
		localEchoVolume.store(file);
		inputSensitivity.store(file);
		ambientVolume.store(file);
		//ambientMusicShuffle.store(file);
		paRelaxation.store(file);
		// Store some remotley controlled settings as well - in case we switch between master and player
		//comMatrix.store(file);
		team.store(file);
		comEffect.store(file); 
		npcVisibility.store(file);
		headTrackerType.store(file);
		headTrackerDevice.store(file);
		voiceEffectConfig.store(file);
		voiceEffectName.store(file);
		activeTab.store(file);
		activeSketchTab.store(file);
		ambientVolume.store(file);
		ambientFade.store(file);
		musicPos.store(file);
		soundFile.store(file);
		soundLoop.store(file);

		file->commit();
		qDebug("Ctrl: Config file written");
	}
	// Set the shutdown-flag to true so all the threads can quit gracefully
	lock.lockForWrite();
	shutdown = true;
	lock.unlock();
	emit(shutDown());
}

bool Ctrl::run(void)
{
	bool r;
	lock.lockForRead();
	r = !shutdown;
	lock.unlock();
	return r;
}

bool Ctrl::briefRemotePlayers()
{
	ctrl->npcVisibility.send();
	ctrl->team.send();
	ctrl->comEffect.send();
	return true; // FIXME: Check whether we really sent it
}

void Ctrl::newRemoteControl(int player, QByteArray data)
// We've got a new remote control command
{
	QStringList stringList;
	ComEffect comeffect;
	qDebug("Ctrl: New remote control command received");
	if (remotePlayerName.get(player) != "GAMEMASTER") {qWarning("Ctrl: Player %d who is not the gamemaster dared to send us remote controls. Ignoring them!", player); return;}
	if (data.length()<1) {qWarning("Ctrl: Got a too short control command sequence"); return;}
	// The first byte holds the information, which control type we're talking about
	Remotes rem = (Remotes)data.at(0);
	data = data.mid(1); // Cut the command off
	switch (rem) {
		case REMOTE_COMEFFECT:
			qDebug() << "Ctrl: Got new control update for comeffect";
			fromText(comeffect, data);
			ctrl->comEffect.set(comeffect);
			emit(commEffectChange()); // To whom this may concern
			break;
		case REMOTE_NPCVISIBILITY: 
			fromText(stringList, data);
			qDebug() << "Ctrl: Got new control update for npcVisibility:" << stringList;
			npcVisibility.set(stringList); 
			emit(npcVisibilityChange()); 
			break;
		case REMOTE_TEAM:
			qDebug() << "Ctrl: Got new team update:" << data.toHex();
			if (data.size()<MAXPLAYERS*sizeof(int)) {qWarning("Ctrl: Too short team remote control!"); return;}
			for (int p=0; p<MAXPLAYERS; p++) {
				int t;
				if (!fromNBO(&data, &t)) {qWarning("Ctrl: Got team remote control and could not decode it"); return;}
				if (t >= MAXTEAMS) {qWarning("Ctrl: Got team remote control with team value out of range"); return;}
				team.set(t, p);
				qDebug("Ctrl: decoded team %d", t);
			}
			emit(teamChange());
			break;
		default:
			qWarning() << "Ctrl: Got unknown control update, type:" << data.at(0) << "???";
	}
}






