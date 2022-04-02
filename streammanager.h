#ifndef STREAMMANAGER_H
#define STREAMMANAGER_H

#include <QThread>
#include <QProcess>
#include <QUdpSocket>
#include <QFile>

#include "config.h"
#include "control.h"
#include "painterface.h"
#include <opus/opus.h>
#include "buffer.h"
#include "headtracker.h"
#include "soundfx.h"
#include "bulktransfer.h"


class StreamManager : public QThread
{
	Q_OBJECT
public:
	StreamManager(BulkBay * b);
	~StreamManager();
protected:
	void run() override;
private:
	void init();
	void demux(QByteArray recbuf, QHostAddress senderAddress, uint16_t senderPort);
	bool startMusicPlayer();
	bool startSoundPlayer();
	bool addMusic(Stereo_16 * data, float amp);
	bool addSound(Stereo_16 * data, float amp);
	Mono * localvoice1, * localvoice2;
	Mono * remotevoice[MAXPLAYERLINKS];
	Stereo * localsoundout;
	Stereo_16 * ambient;
	Stereo_16 * dummy;
	QUdpSocket * udpSocket; //, * udpSocketIpv6;
	OpusEncoder * voiceEncoder1;
	OpusEncoder * voiceEncoder2;
	OpusEncoder * ambientEncoder;
	OpusDecoder * voiceDecoder[MAXPLAYERLINKS];
	OpusDecoder * ambientDecoder;
	uint16_t ambientpacketcount[MAXPLAYERLINKS];
	uint16_t voicepacketcount[MAXPLAYERLINKS];
	SendBuf sendbuf[MAXPLAYERLINKS];
	StreamFifoArray * remoteVoiceFifo;
	StreamFifoArray * remoteAmbientFifo;
	Headtracker * tracker;
	SoundFx * soundFx;
	BulkBay * bulkbay;
	int cyclecount;
	int idletime[MAXPLAYERLINKS];
	QList<int> rxpackets, txbytes, rxbytes;
	bool ambientPlaying;
	ComEffect comEffect;
	QProcess * musicPlayer, * soundPlayer;
private slots:
	void loop();
};

extern StreamManager * streammanager;

#endif