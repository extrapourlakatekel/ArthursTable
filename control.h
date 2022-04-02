#ifndef CTRL_H
#define CTRL_H
#include <QHostAddress>
#include <QtNetwork>
#include <QThread>
#include <QDir>
#include <QObject>
#include <QSaveFile>
#include <QFile>
#include <QProcess>
#include <QMutex>
#include <assert.h>
#include "config.h"
#include "types.h"
#include "convert.h"
#include "bulktransfer.h"

void broadcast(QByteArray data);
void sendout(int player, QByteArray data);

template <class T> 
class Local : public QObject {
public:
	Local(const char * name, T initial, int entries = 1);
	T get(int entry = 0);
	bool set(T value, int entry = 0);
	void store(QSaveFile * file);
	void load(QFile * file);
protected:
	QByteArray n;
	int e;
	QReadWriteLock lock;
	T * val;
	T dflt;
};

template <class T>
Local<T>::Local(const char * name, T initial, int entries) : QObject()
{
	if (entries<0) {qDebug() << "Local" << n << "::Local: assertation 'entries>=0' failed!"; qFatal("Exiting!");}
	dflt = initial;
	val = new T[entries];
	e = entries;
	for (int i=0; i<e; i++) val[i] = initial;
	n = name;
}

template <class T>
T Local<T>::get(int entry)
{
	T v;
	//qDebug("Ctrl.get: %d / %d", entry, e);
	if (entry>=e || entry<0) {qDebug() << "Local" << n << "::get: assertation 'entry >= 0 / entry <" << e << "' failed with entry =" << entry; qFatal("Exiting!");}
	lock.lockForRead();
	v = val[entry];
	lock.unlock();
	return v;
}

template <class T>
bool Local<T>::set(T value, int entry) 
// Set the variable with locking and return true if changes occured
{
	bool changes = false;
	//qDebug() << n << entry << e;
	if (entry>=e || entry<0) {qDebug() << "Local" << n << "::set: assertation 'entry >= 0 / entry <" << e <<"' failed with entry =" << entry; qFatal("Exiting!");}
	lock.lockForWrite();
	if (val[entry] != value) {
		val[entry] = value;
		changes = true;
	}
	lock.unlock();
	return changes;
}

template <class T>
void Local<T>::load(QFile * file)
{
	QByteArray line, right;
	int index;
	file->seek(0);
	while (!file->atEnd()) {
		line = file->readLine();
		index = line.indexOf('=');
		if (index > 0) {
			//qDebug(line.left(index));
			if (line.left(index) == n) {
				// Found own identifier
				right = line.mid(index+1);
				//qDebug() << " Right: " << right;
				for (int entry = 0; entry < e; entry++) {
					index = right.indexOf(',');
					if (index < 0) index = right.length();
					//qDebug() << entry << ":" << right.left(index).simplified();
					if (!fromText(val[entry], right.left(index).simplified())) val[entry] = dflt; // If anything bad happens during file load, use the default value
					right = right.mid(index+1);
				}
				break;
			}
		}
	}
}

template <class T>
void Local<T>::store(QSaveFile * file)
{
	file->write(n);
	file->write("=");
	for (int entry = 0; entry < e; entry++) {
		QByteArray t = toText(val[entry]);
		// Simply remove characters that may cause problems - don't care whether anybody needs them!
		t.replace(',',' ');	t.replace('\n',' '); t.replace('=',' '); 
		file->write(t);
		if (entry < e-1) file->write(",");
	}
	file->write("\n");
}

template <class T> 
class Remote : public Local<T> {
public:
	Remote(const char * name, uint8_t identity, T initial, int entries = 1);
	bool set(T value, int entry=0);
	void send();
	void receive(QByteArray);
private:
	uint8_t id;
};

template <class T>
Remote<T>::Remote(const char * name, uint8_t identity, T initial, int entries) : Local<T>(name, initial, entries)
{
	// Remote features an additional ID
	id = identity;
}

template <class T>
bool Remote<T>::set(T value, int entry)
{
	//qDebug() << id << entry << value;
	// This entity cannot know whether we are gamemaster or player. The calling instance has to assure, that only the gamemaster can set remote control values
	if (Local<T>::set(value, entry)) {send(); return true;} // Send only when a change occured
	return false;
}

template <class T>
void Remote<T>::send()
// Send out the control variable to all corresponding players
{
	// Now that's a little bit tricky. 
	// Easy: When we hold one entry, we assume that it has to be sent to everybody.
	if (Local<T>::e == 1) {
		QByteArray data(1, (char) id);
		data.append(toText(Local<T>::val[0]));
		qDebug("Ctrl: Sending onedimensional data, id: %d", id);
		broadcast(data);
	}
	// Medium: When we hold MAXPLAYERLINKS entries, we assume that each entry has to go to the corresponding player
	else if (Local<T>::e == MAXPLAYERLINKS) {
		for (int p=0; p < MAXPLAYERLINKS; p++) {
			QByteArray data(1, (char) id);
			data.append(toText(Local<T>::val[p]));
			sendout(p, data);
		}
	} 
	// Tricky: When we hold MAYPLAYER entries, send them in a way that everyone gets the proper MAXPLAYER_LINKS_ entries only
	else if (Local<T>::e == MAXPLAYERS) {
		for (int p=0; p<MAXPLAYERLINKS; p++) {
			QByteArray data(1, (char) id);
			data.append(toNBO(Local<T>::val[p+1])); // The player has his own status first in row
			data.append(toNBO(Local<T>::val[0])); // Then he lists the gamemaster
			for (int k=0; k<MAXPLAYERLINKS; k++) if (k != p) {
				data.append(toNBO(Local<T>::val[k+1])); // The he lists all the others in the same order
			}
			sendout(p, data);
		}
	}
	/* // With the death of the commatrix-unit obsolete
	// And the hard part... When it's a vector of this size we assume it'a s matrix and every player gets his vector
	else if (Local<T>::e == MAXPLAYERS * MAXPLAYERLINKS) {
		int entry = MAXPLAYERLINKS; // Don't send the settings for the local player
		for (int p=0; p<MAXPLAYERLINKS; p++) {
			QByteArray data(1, (char) id);
			for (int k=0; k<MAXPLAYERLINKS; k++) {
				QByteArray temp = toText(Local<T>::val[entry++]);
				data.append((char)temp.size());
				data.append(temp);
			}
			sendout(p, data);
		}
	}
	 */
}

template <class T>
void Remote<T>::receive(QByteArray data)
// This function is called, when a remote control command has been received
{
	// The first one's easy. When we hold one element, the received value will go straight into it.
	if (Local<T>::e == 1) fromText(Local<T>::val[0], data);
	// When we hold MAXPLAYERLINKS entries, the gamemaster sends out one element to each player. We receive, so we're this single player.
	// For us only the first entry will be used and has to be filled with the received value
	else if (Local<T>::e == MAXPLAYERLINKS) fromText(Local<T>::val[0], data);
	else {} // That will be thought about later...
}


class Ctrl : public QObject
// Global control variables to simplify data excange between the threads and to the players
{
	Q_OBJECT
public:
	enum State {STOPPED, STARTUP, RUNNING, FADETOSHUTDOWN, SHUTDOWN, FADETORESTART, RESTART};
	Ctrl();
	//~Ctrl();
	QDir basedir;
	QDir dir[NDIRS]; 
	QVector<Dirs> dirstosend;
	void setTableName(QString name);
	Local<int> mode 						= {"mode", MODE_PLAYER};
	Local<QString> punchingServer 			= {"punchingserver", "https://robinsnest.de/cgi-bin/puncher.pl"};
	Local<bool> useServer               	= {"useserver", false};
	Local<QString> remotePlayerName 		= {"remoteplayername", "", MAXPLAYERLINKS};
	Local<char> playerSign					= {"remoteplayerSign", '?', MAXPLAYERS};
	Local<int> remotePlayerVolume			= {"remoteplayervolume", 50, MAXPLAYERLINKS};
	Local<QHostAddress> remotePlayerAddress = {"remoteplayeraddress", QHostAddress(""), MAXPLAYERLINKS};
	Local<bool> remoteAddressOverride		= {"remoteaddressoverride", false, MAXPLAYERLINKS};
	Local<int> remotePlayerPort				= {"remoteplayerport", PORTNUMBER, MAXPLAYERLINKS};
	Local<QString> localPlayerName			= {"localplayername", ""};
	Local<QString> tablename				= {"tablename", ""};
	Local<QByteArray> passPhrase			= {"passphrase", ""};
	Local<QByteArray> packetHeader			= {"packetheader", PACKETHEADER};
	Local<bool> ipv4Available				= {"ipv4available", false};
	Local<bool> ipv6Available				= {"ipv6available", false};
	Local<ConStatus> connectionStatus		= {"connectionstatus", CON_DISCONNECTED, MAXPLAYERLINKS};
	Local<bool> broadcastAmbient			= {"broadcastambient", true, MAXPLAYERLINKS};
	Local<float> heading					= {"heading", 0.0};
	Local<int> voiceEffectSelected			= {"voiceeffectselected", -1}; // Hint: All players may have voice effects not only the remote ones
	Local<QByteArray> voiceEffectConfig     = {"voiceeffectconfig", "", VOICEPRESELECTIONS}; 
	Local<QString>  voiceEffectName			= {"voiceeffectname", "", VOICEPRESELECTIONS};
	Local<int> localEchoVolume				= {"localechovolume", 0};
	Local<Mono> vuLocalVoice				= {"vulovalvoice", 0.0};
	Local<Stereo> vuOut						= {"vuout", Stereo(0.01, 0.01)};
	Local<Stereo> vuAmbient					= {"vuambient", Stereo(0.01, 0.01)};
	Local<bool> localVoiceActive			= {"localvoiceactive", false};
	Remote<bool> dominantAmbient			= {"dominantambient", REMOTE_DOMINANTAMBIENT, false};
	Local<int> inputSensitivity				= {"inputsensitivity", 0};
	Local<State> musicState					= {"musicstate", STOPPED};
	Local<QStringList> musicList			= {"musiclist", QStringList()};
	Local<int> musicPos						= {"musicpos", 0};
	Local<QDir> musicDir					= {"musicdir", QDir()};
	Local<State> soundState					= {"aoundstate", STOPPED};
	Local<QString> soundFile				= {"soundfile", QString()};
	Local<bool> soundLoop					= {"ambientloop", false};
	Local<int> ambientFade					= {"ambientfade", 50};
	Local<int> ambientVolume				= {"ambientvolume", 50};
	Local<int> paRelaxation					= {"parelaxation", 2};
	Remote<int> team				 		= {"team", REMOTE_TEAM, 0, MAXPLAYERS};
	Remote<ComEffect> comEffect				= {"comeffect", REMOTE_COMEFFECT, COMEFFECT_NONE};
	Remote<QStringList> npcVisibility		= {"npcvisibility", REMOTE_NPCVISIBILITY, QStringList()};
	Local<TrackerType> headTrackerType      = {"headtrackertype", TRACKER_NONE};
	Local<QString> headTrackerDevice        = {"headtrackerdevice", ""};
	Local<bool> mute                        = {"mute", false};
	Local<bool> isSendingVoice				= {"isSendingVoice", false, MAXPLAYERLINKS};
	Local<bool> pushToTalk 					= {"pushtotalk", false};
	Local<int> activeTab					= {"activetab", 0, MAXTEAMS};
	Local<bool> facilityManagerReady		= {"facilitymanagerready", false};
	Local<int> activeSketchTab				= {"activesketchtab", 0};
	void quit(void);
	bool run(void);
	bool briefRemotePlayers();
private:
	QDir createDir(QString name);
	QReadWriteLock lock;
	bool shutdown;
public slots:
	void newRemoteControl(int player, QByteArray data);
signals:
	void gamemasterChange(); // Local player is now gamemaster or not - this may require some layout changes
	void remotePlayerNameChange(int); // Simply signal the change and the affected player, the new values are found in the control variable
	void remotePlayerAddressChange(int); // Simply signal the change and the affected player, the new values are found in the control variable
	void playerSignChange(int); // The players are numbered, when the names change soon thereafter the numbers change as well
	void localPlayerNameChange();
	void teamChange();
	void commEffectChange();
	void voiceEffectChange(int effectNr);
	void connectionStateChange(int);
	void npcVisibilityChange();
	void mapVisibilityChange();
	void functionKeyPressed(int);
	void functionKeyReleased(int);
	void escKeyPressed();
	void escKeyReleased();
	void mainLayoutChange();
	void initHeadtracker(); // Some changes have been made to the tracker configuration, (re)init it!
	void fileTransferStart(int index, QString name, int player);
	void fileTransferProgress(int index, int percentage);
	void fileTransferComplete(int);
	void statistics(QList<int>, QList<int>, QList<int>);
	void resetConnection(int player);
	void configureEffect(int);
	void localEchoVolumeChange();
	void muteChange();
	void chatCommand(QString); // Emitted, when a command is entered in the chat window
	void displayText(QString, int, bool); // To display text in the chat window
	void checkNewFiles();
	void fileUpdate(int, QString);
	void shutDown();

	//void ctrlUpdateNpcVisibility(); // No parameters required, the control variable is already set apropriately
	//void ctrlUpdateMapVisibility();
};

extern Ctrl * ctrl; // Yes, I know - global instances are poor...

#endif