#include <assert.h>
#include "streammanager.h"
#include <QSerialPort>
#include <QEventLoop>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>

#define PACKETDEBUG(x)

StreamManager::StreamManager(BulkBay * b) : QThread()
{
	qDebug() << "Initializing StreamManager...";
	bulkbay = b;
	// Must be done within the custructor to be run within main thread

}

StreamManager::~StreamManager()
{
	delete udpSocket;
//	delete udpSocketIpv6;
}

void StreamManager::init()
{
	// Constructor is called from a different thread - this causes problems when variables are not initialized here
	// Do some variable initialisations and memory allocation
	int error;

	this->setObjectName("gotongi RT thread");

	udpSocket = new QUdpSocket(this);
	if (!udpSocket->bind(QHostAddress::Any, PORTNUMBER, QAbstractSocket::ReuseAddressHint)) {
		qFatal("StreamManager Init: Could not bind to socket for port %d! Reported error is %s", PORTNUMBER, (char*)udpSocket->errorString().data());
	}

	remoteVoiceFifo = new StreamFifoArray(MAXPLAYERLINKS, VOICEFIFOLENGTH);
	remoteAmbientFifo = new StreamFifoArray(1, AMBIENTFIFOLENGTH);
	
	for (int i=0; i<MAXPLAYERLINKS; i++) {
		voicepacketcount[i] = 0;
		ambientpacketcount[i] = 0;
		sendbuf[i].clear(ctrl->packetHeader.get()); // Prepare the first packet to send
		remotevoice[i] = new Mono[FRAMESIZE * BUNDLE];
		memset((void *)remotevoice[i], 0, FRAMESIZE * BUNDLE * sizeof(Mono));
		idletime[i] = IDLETIME;
		rxpackets << 0;
		txbytes << 0;
		rxbytes << 0;
	}

	localvoice1 = new Mono[FRAMESIZE * BUNDLE];
	assert(localvoice1 != NULL);
	memset((void *)localvoice1, 0, FRAMESIZE * BUNDLE * sizeof(Mono));

	localvoice2 = new Mono[FRAMESIZE * BUNDLE];
	assert(localvoice2 != NULL);
	memset((void *)localvoice2, 0, FRAMESIZE * BUNDLE * sizeof(Mono));
	
	localsoundout = new Stereo[FRAMESIZE * BUNDLE];
	assert(localsoundout != NULL);
	memset((void *)localsoundout, 0, FRAMESIZE * BUNDLE * sizeof(Stereo));
	
	// Initialize buffer for uncompressed ambient sound, will be read from pipe later, we do not process this so we can stay in Stereo_16
	ambient = new Stereo_16[FRAMESIZE * BUNDLE]; 
	assert(ambient != NULL);
	memset((void *)ambient, 0, FRAMESIZE * BUNDLE * sizeof(Stereo_16));

	// Need another buffer
	dummy = new Stereo_16[FRAMESIZE * BUNDLE]; 
	assert(dummy != NULL);

	// *** Initialize sound compression and decompression ***
	// Local voice encoder
	error = OPUS_OK;
	voiceEncoder1 = opus_encoder_create((opus_int32)SAMPLERATE, MONO, OPUS_APPLICATION_AUDIO, &error); // AUDIO mode is chosen for better transmission of modified voices
	if (error != OPUS_OK) qFatal("StreamManager Init: Could not initialize Opus voice encoder! Error: %d", error);
	opus_encoder_ctl(voiceEncoder1, OPUS_SET_BITRATE(VOICEBITRATE));
	opus_encoder_ctl(voiceEncoder1, OPUS_SET_INBAND_FEC(1));
	opus_encoder_ctl(voiceEncoder1, OPUS_SET_SIGNAL(OPUS_AUTO)); // Yes, I know, it's voice that is beeing transmitted, but with voice effect we do need the fullbandwidth experience
	opus_encoder_ctl(voiceEncoder1, OPUS_SET_COMPLEXITY(10));
	
	// Local encoder for transfer effect voice
	error = OPUS_OK;
	voiceEncoder2 = opus_encoder_create((opus_int32)SAMPLERATE, MONO, OPUS_APPLICATION_VOIP, &error);
	if (error != OPUS_OK) qFatal("StreamManager Init: Could not initialize Opus voice with effect encoder! Error: %d", error);
	opus_encoder_ctl(voiceEncoder2, OPUS_SET_BITRATE(VOICEBITRATE));
	opus_encoder_ctl(voiceEncoder2, OPUS_SET_INBAND_FEC(1));
	opus_encoder_ctl(voiceEncoder2, OPUS_SET_SIGNAL(OPUS_AUTO)); // Yes, I know, it's voice that is beeing transmitted, but with voice effect we do need the fullbandwidth experience
	opus_encoder_ctl(voiceEncoder2, OPUS_SET_COMPLEXITY(10));
	
	// Ambient encoder (only required when we're master, but let's do it anyway
	error = OPUS_OK;
	ambientEncoder = opus_encoder_create((opus_int32)SAMPLERATE, STEREO, OPUS_APPLICATION_AUDIO, &error);
	if (error != OPUS_OK) qFatal("StreamManager Init: Could not initialize Opus ambient encoder! Error: %d", error);
	opus_encoder_ctl(ambientEncoder, OPUS_SET_BITRATE(AMBIENTBITRATE));
	opus_encoder_ctl(ambientEncoder, OPUS_SET_INBAND_FEC(1));
	opus_encoder_ctl(ambientEncoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC));
	opus_encoder_ctl(ambientEncoder, OPUS_SET_COMPLEXITY(10));
	
	// Honestly, I dont't want to change that dynamically during runtime, so occupy decoder space for the maximum number of players!
	error = OPUS_OK;
	for (int i=0; i<MAXPLAYERLINKS; i++) {
		voiceDecoder[i] = opus_decoder_create((opus_int32)SAMPLERATE, MONO, &error); 
		if (error != OPUS_OK) qFatal("StreamManager Init: Could not initialize Opus voice decoder Nr. %d Error: %d\r\n", i, error);
	}
	error = OPUS_OK;
	ambientDecoder = opus_decoder_create((opus_int32)SAMPLERATE, STEREO, &error);
	if (error != OPUS_OK) qFatal("StreamManager Init: Could not initialize Opus ambient sound decoder! Error: %d", error);
	
	musicPlayer = new QProcess(); // FIXME: Proably we'll have an own player in the far future
	musicPlayer->setReadChannel(QProcess::StandardOutput);
	musicPlayer->setProcessChannelMode(QProcess::SeparateChannels);

	soundPlayer = new QProcess(); // FIXME: Proably we'll have an own player in the far future
	soundPlayer->setReadChannel(QProcess::StandardOutput);
	soundPlayer->setProcessChannelMode(QProcess::SeparateChannels);

	soundFx = new SoundFx();
	//qDebug("Done");
}

void StreamManager::run()
{
	init();
	QEventLoop eventLoop(this);
	// Loop till program wants to end
	qDebug("StreamManager: Started thread");
	
	// Create a file with the process ID of streammanager to be able to priorize this process
	// (not strictly required, can be omitted for other OS)
	QFile file(ctrl->basedir.filePath("streammanager_pid.txt"));
	file.open(QIODevice::WriteOnly);
	pid_t tid = syscall(SYS_gettid);
	QByteArray text;
	text.setNum(tid);
	file.write(text+"\n");
	file.close();
	qDebug("StreamManager: Waiting for FacilityManager to become ready");
	while (!ctrl->facilityManagerReady.get()) usleep(10000);
	qDebug("StreamManager: FacilityManager ready!");

	qDebug("StreamManager: Initializing Pulseaudio with relaxation of %d", ctrl->paRelaxation.get());
	if (!pa_init(NULL, NULL, FRAMESIZE, SAMPLERATE, ctrl->paRelaxation.get())) qFatal("StreamManager Loop: Cannot initialize audio!");
	//memset((void *)ambient, 0, FRAMESIZE * STEREO * sizeof(int16_t) * BUNDLE);

	QTimer timer;
	timer.setInterval(1);
	timer.setSingleShot(false);
	connect(&timer, SIGNAL(timeout()), this, SLOT(loop()));
	//timer.start();
	qDebug("StreamManager: Starting loop");
	while (ctrl->run()) {loop();}
	qDebug() << "StreamManager: stopped";
	qDebug() << "StreamManager: Pulseaudio deactivated";
}

bool StreamManager::addMusic(Stereo_16 * data, float amp)
{
	static float fadelevel = 1.0;
	int musicstate = ctrl->musicState.get();
	if (musicstate != Ctrl::STOPPED && ctrl->mode.get() != MODE_MASTER) ctrl->musicState.set(Ctrl::SHUTDOWN); // When mode changed from master to player, disable music playing
	// Make local copies of some control variables to protect them from changing during one run causing glitches
	//qDebug("StreamManager: Loop start");
	// Get the ambient music from stdout of mpg123
	// When we are in player mode we can expect to get the ambient sound via stream...
	//qDebug("StreamManager: music state %d", ctrl->musicState.get());
	bool fade = false;
	//qDebug("Streammanager: Music state is: %d", musicstate);
	switch (musicstate) {
		case Ctrl::STOPPED: 
			return false;
		case Ctrl::STARTUP:
			if (!startMusicPlayer()) {ctrl->musicState.set(Ctrl::STOPPED); return false;}
			ctrl->musicState.set(Ctrl::RUNNING);

			fadelevel = 1.0;
			return false;
		case Ctrl::FADETOSHUTDOWN:
		[[fallthrough]];
		case Ctrl::FADETORESTART:
		fade = true;
		[[fallthrough]];
		case Ctrl::RUNNING:
			if (!musicPlayer->isOpen() || musicPlayer->read((char *)dummy, FRAMESIZE * BUNDLE * sizeof(Stereo_16)) < (unsigned)(FRAMESIZE * BUNDLE * sizeof(Stereo_16))) {
				// We shall provide music and cannot? Provide high quality silence instead!
				qDebug("StreamManger: Cannot read enough data from music pipe");
				memset((void *)dummy, 0, FRAMESIZE * STEREO * sizeof(int16_t) * BUNDLE);
				// If we ran dry, check whether we should restart the player with the next entry
				if (musicPlayer->state() == QProcess::NotRunning) {
					ctrl->musicPos.set((ctrl->musicPos.get()+1) % ctrl->musicList.get().size());
					ctrl->musicState.set(Ctrl::STARTUP);
				}
			}
			if (fade) {
				for (int i=0; i<FRAMESIZE * BUNDLE; i++) {
					dummy[i] *= fadelevel; 
					if (fadelevel >= 0.001 && (fadelevel *= 0.9998) < 0.001) {
						// The moment the fading is done change the states but keep fading for the rest of the frame
						if (musicstate == Ctrl::FADETOSHUTDOWN) ctrl->musicState.set(Ctrl::SHUTDOWN);
						else ctrl->musicState.set(Ctrl::RESTART); 
					}
				}
			}
			for (int i=0; i<FRAMESIZE * BUNDLE; i++) data[i] += dummy[i] * amp;
			return true;
		case Ctrl::SHUTDOWN:
			if (musicPlayer->state() != QProcess::NotRunning) musicPlayer->kill();
			else {
				if (musicPlayer->isOpen() ) musicPlayer->readAllStandardOutput(); // Drain all music data
				ctrl->musicState.set(Ctrl::STOPPED);
			}
			return false; 
		case Ctrl::RESTART:
			//qDebug("Streammanager: Restarting");
			if (musicPlayer->state() != QProcess::NotRunning) musicPlayer->kill();
			else {
				if (musicPlayer->isOpen() ) musicPlayer->readAllStandardOutput(); // Drain all music data
				ctrl->musicState.set(Ctrl::STARTUP);
			}
			fadelevel = 1.0;
			return false; 
		default:
			qWarning("Streammanager: addMusic reached unkown state!");
			return false;
	}	
}

bool StreamManager::addSound(Stereo_16 * data, float amp)
{
	static float fadelevel = 1.0;
	int soundstate = ctrl->soundState.get();
	bool soundloop = ctrl->soundLoop.get();
	if (soundstate != Ctrl::STOPPED && ctrl->mode.get() != MODE_MASTER) ctrl->soundState.set(Ctrl::SHUTDOWN); // When mode changed from master to player, disable music playing
	// Make local copies of some control variables to protect them from changing during one run causing glitches
	//qDebug("StreamManager: Loop start");
	// Get the ambient music from stdout of mpg123
	// When we are in player mode we can expect to get the ambient sound via stream...
	//qDebug("StreamManager: music state %d", ctrl->musicState.get());
	bool fade = false;
	//qDebug("Streammanager: Sound state is: %d", soundstate);
	switch (soundstate) {
		case Ctrl::STOPPED: 
			return false;
		case Ctrl::STARTUP:
			if (!startSoundPlayer()) {ctrl->soundState.set(Ctrl::STOPPED); return false;}
			ctrl->soundState.set(Ctrl::RUNNING);
			fadelevel = 1.0;
			return false;
		case Ctrl::FADETOSHUTDOWN:
		[[fallthrough]];
		case Ctrl::FADETORESTART:
		fade = true;
		[[fallthrough]];
		case Ctrl::RUNNING:
			if (!soundPlayer->isOpen() || soundPlayer->read((char *)dummy, FRAMESIZE * BUNDLE * sizeof(Stereo_16)) < (unsigned)(FRAMESIZE * BUNDLE * sizeof(Stereo_16))) {
				// We shall provide music and cannot? Provide high quality silence instead!
				qDebug("StreamManger: Cannot read enough data from sound pipe");
				memset((void *)dummy, 0, FRAMESIZE * STEREO * sizeof(int16_t) * BUNDLE);
				// If we ran dry, set state to stopped
				if (soundPlayer->state() == QProcess::NotRunning) {
					if (soundloop) ctrl->soundState.set(Ctrl::STARTUP);
					else ctrl->soundState.set(Ctrl::STOPPED);
				}
			}
			if (fade) {
				for (int i=0; i<FRAMESIZE * BUNDLE; i++) {
					dummy[i] *= fadelevel; 
					if (fadelevel >= 0.001 && (fadelevel *= 0.9998) < 0.001) {
						// The moment the fading is done change the states but keep fading for the rest of the frame
						if (soundstate == Ctrl::FADETOSHUTDOWN) ctrl->soundState.set(Ctrl::SHUTDOWN);
						else ctrl->soundState.set(Ctrl::RESTART); 
					}
				}
			} else {
				// We're not already fading to next sound or to stop and the buffer drains? Start player again!
				if (soundPlayer->bytesAvailable() < 256000 && soundPlayer->state() == QProcess::NotRunning && soundloop) {
					qDebug("Streammanager: Restarting ambient sound");
					ctrl->soundState.set(Ctrl::STARTUP);
				}
			}
			for (int i=0; i<FRAMESIZE * BUNDLE; i++) data[i] += dummy[i] * amp;
			return true;
			
		case Ctrl::SHUTDOWN:
			if (soundPlayer->state() != QProcess::NotRunning) soundPlayer->kill();
			else {
				if (soundPlayer->isOpen() ) soundPlayer->readAllStandardOutput(); // Drain all music data
				ctrl->soundState.set(Ctrl::STOPPED);
			}
			return false; 
		case Ctrl::RESTART:
			//qDebug("Streammanager: Restarting");
			if (soundPlayer->state() != QProcess::NotRunning) soundPlayer->kill();
			else {
				if (soundPlayer->isOpen() ) soundPlayer->readAllStandardOutput(); // Drain all music data
				ctrl->soundState.set(Ctrl::STARTUP);
			}
			fadelevel = 1.0;
			return false; 
		default:
			qWarning("Streammanager: addSound reached unkown state!");
			return false;
	}	
}


void StreamManager::loop() 
{
	int length;
	int error;
	static int framessent = 0;
	QByteArray localvoiceenc1;
	QByteArray localvoiceenc2;
	QByteArray ambientenc;
	QByteArray remoteambientenc;
	QByteArray remotevoiceenc;
	timeval starttime, actualtime;
	int delta;
	
	gettimeofday(&starttime, NULL); // For profiling purposes only
	
	comEffect = ctrl->comEffect.get();

	// Zero the ambient sound array
	memset((void *)ambient, 0, FRAMESIZE * BUNDLE * sizeof(Stereo_16));
	// Get music
	float fade = ctrl->ambientFade.get()/100.0;
	ambientPlaying = false;
	if (addMusic(ambient, 1.0 - fade)) ambientPlaying = true;
	if (addSound(ambient, fade)) ambientPlaying = true;
	
	// For local echo, an as short as possible delay is important to recognize one's own voice - for remote players we have more time to waste
	// So process short frames fast and bundle some before transmitting
	// We do manual loop unrolling here to use processing time while waiting for sound data package 

	// Process the first dataset
	//qDebug("Requesting new audio frame");
	// Get new pointer with voice data from soundcard and the pointer to shove in new data to output. Blocks when no data is available.
	// This causes high CPU load on one core but ensures fastest response times
	// Bundle with the decoded input voice streams from the other players and the decoded ambient sound ... 

	pa_readframe(&localvoice1[FRAMESIZE * 0]);
	soundFx->run(localvoice1, remotevoice, ambient, localsoundout, 0);
	pa_writeframe((float*)&localsoundout[FRAMESIZE * 0]);

	// Use the time till the soundcards delivers the next dataset to process things
	QCoreApplication::processEvents(); // We do not have an own event loop and QProcess does not work without so do it here while we have time - usually doesn't take long
	// Encode ambient sound an music - if nessecairy!
	if (ambientPlaying)  {
		ambientenc.resize(AMBIENTBYTESPERFRAME * BUNDLE);
		// This encoder works on int16 only!
		length = opus_encode(ambientEncoder, (opus_int16*)ambient, FRAMESIZE * BUNDLE, (unsigned char *)ambientenc.data(), AMBIENTBYTESPERFRAME * BUNDLE);
		if (length < 0) qFatal("StreamManager: OPUS cannot encode ambient! Error: %d", length);
		ambientenc.resize(length);
	}
	// Check for incoming data - process all available packets
	// Remark: Yes, I know that this could be done using event driven processing, but I want to have it processed exactly here!
	QByteArray recbuf;
	QHostAddress senderAddress;
	quint16 senderPort;
	// Check for datagrams
	while (udpSocket->hasPendingDatagrams()) {
		//qDebug() << "IPv4-UDP-Packet received";
		int size = udpSocket->pendingDatagramSize();
		recbuf.resize(size);
		udpSocket->readDatagram(recbuf.data(), recbuf.size(), &senderAddress, &senderPort);
		// Demux the incoming packets to the input buffers and add acknowledgements for the receipt packets where required
		//qDebug() << "Receivebuffer:" << recbuf.toHex();
		if (size > MAXDATAGRAMSIZE || size <= 0) qWarning("StreamManager: Received datagram with strange size!");
		else demux(recbuf, senderAddress, senderPort);
	}


	// Process the second dataset
	pa_readframe(&localvoice1[FRAMESIZE * 1]);
	soundFx->run(localvoice1, remotevoice, ambient, localsoundout, 1);
	pa_writeframe((float*)&localsoundout[FRAMESIZE * 1]);
	 
	//qDebug("StreamManager: Doing the network stuff");

	cyclecount = (cyclecount+1) % PINGTIME; // The cyclecounter is required to define the ping interval when a remote player is not responding yet
	if (cyclecount == 0) bulkbay->allowResend();
	bool enc1ready = false;
	bool enc2ready = false;
	for (int player=0; player<MAXPLAYERLINKS; player++) {
		//qDebug() << "StreamManager: processing data for player" << player;
		QHostAddress address = ctrl->remotePlayerAddress.get(player);
		//qDebug() << "address is" << address.toString();
		// Skip if no address is configured for that player
		if (!address.isNull()) {
			if (ctrl->connectionStatus.get(player) == CON_READY) {
				// We do have a basic connection but there are other reasons that may forbid sending data
				if (!ctrl->mute.get()) {
					// When we're mute do not send voice packets at all
					if (comEffect == COMEFFECT_NONE || ctrl->team.get(0) == ctrl->team.get(player+1)) {
						// When we're in the same team or there is no communication limit, send the normal voice
						// Do encoding when not already done
						if (!enc1ready) {
							// Encode the local voice data only when required
							//qDebug("StreamManager: Encoding local voice");
							localvoiceenc1.resize(VOICEBYTESPERFRAME * BUNDLE); // Allow for maximum possible frame length
							length = opus_encode_float(voiceEncoder1, localvoice1, FRAMESIZE * BUNDLE, (unsigned char *)localvoiceenc1.data(), VOICEBYTESPERFRAME * BUNDLE);
							if (length < 0) qFatal("StreamManager: OPUS cannot encode localvoice! Error: %d", length);
							localvoiceenc1.resize(length); // Shorten to real frame length
							enc1ready = true; // Even if more than one player wants the data encoding it once is sufficient
						}
						PACKETDEBUG(qDebug("StreamManager: Adding voice packet with length %d", localvoiceenc1.size());)
						sendbuf[player].append(PACKET_VOICE, voicepacketcount[player], localvoiceenc1); // FIXME: Error handling!
						voicepacketcount[player]++; 
					}
					else if (ctrl->team.get(0) != ctrl->team.get(player+1) && comEffect != COMEFFECT_MUTE && ctrl->pushToTalk.get()) {
						// Well, we do not share the same team and require special comm effect...
						// Do encoding when not already done
						if (!enc2ready) {
							soundFx->applyComEffect(localvoice1, localvoice2, comEffect, FRAMESIZE * BUNDLE);
							// When we encode radio or telephone effects, limit the bandwidth in the encoder so we don't need a seperate filter in the effect engine
							if (comEffect == COMEFFECT_PHONE || comEffect == COMEFFECT_RADIO) opus_encoder_ctl(voiceEncoder2, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_NARROWBAND));
							else opus_encoder_ctl(voiceEncoder2, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND));
							localvoiceenc2.resize(VOICEBYTESPERFRAME * BUNDLE); // Allow for maximum possible frame length
							length = opus_encode_float(voiceEncoder2, localvoice2, FRAMESIZE * BUNDLE, (unsigned char *)localvoiceenc2.data(), VOICEBYTESPERFRAME * BUNDLE);
							if (length < 0) qFatal("StreamManager: OPUS cannot encode localvoice with comm effect! Error: %d", length);
							localvoiceenc2.resize(length); // Shorten to real frame length
							enc2ready = true;
						}
						PACKETDEBUG(qDebug("StreamManager: Adding voice packet with length %d", localvoiceenc2.size());)
						sendbuf[player].append(PACKET_VOICE, voicepacketcount[player], localvoiceenc2); // FIXME: Error handling!
						voicepacketcount[player]++; 
					}
					// The only options left means not in the same team or MUTE as comm effect or PTT required and not pressed. 
					// In Either case: don't send no packet.
				}
				// Add ambient sound to selected output streams (ambient sound may be produced locally or forwarded)
				if (ambientPlaying) {
					// Do not send ambient packages when we do not stream music
					//qDebug("Adding ambient packet with length %d", ambientenc.size());
					sendbuf[player].append(PACKET_AMBIENT, ambientpacketcount[player], ambientenc); // FIXME: Error handling!
					ambientpacketcount[player]++;
				}
				// Add other data packets to output streams
				QByteArray sniplet;
				//qDebug("Player %d, free space in buffer: %d", player, sendbuf[player].freeSpace());
				while (sendbuf[player].freeSpace() >= SNIPLETSIZE && bulkbay->getNextSniplet(player, sniplet)) {
					 // When we do have a connection: As long as there is data to send and space in the sendbuffer proceed with filling
					PACKETDEBUG(qDebug("StreamManager: Adding sniplet with length %d", sniplet.size());)
					sendbuf[player].append(sniplet);
				}
				txbytes[player] += sendbuf[player].size();
				// And kick the buffer out!
				error = udpSocket->writeDatagram(sendbuf[player].get(), address, ctrl->remotePlayerPort.get(player));
				if (error < 0) qWarning() << "StreamManager Loop: Cannot send buffer to player" << player << udpSocket->errorString();
				//qDebug() << "Sendbuffer:   " << sendbuf[i].get().toHex();

				idletime[player]++; 
				if (idletime[player] == IDLETIME) {
					ctrl->connectionStatus.set(CON_DISCONNECTED, player);
					emit(ctrl->resetConnection(player));
					emit(ctrl->connectionStateChange(player));
					qDebug("StreaManager: Lost connection to player %d", player);
				}
			} else if (cyclecount == 0) {
				// Player is not responding - send only a ping!
				//qDebug() << "Sending ping packet to player" << i << "address" << address.toString();
				sendbuf[player].clear(ctrl->packetHeader.get());
				sendbuf[player].append(QByteArray(1, PACKET_PING));
				sendbuf[player].append(QByteArray(1, (char)(unsigned int)ctrl->localPlayerName.get().toUtf8().size()));
				sendbuf[player].append(ctrl->localPlayerName.get().toUtf8()); // Tell them who we are!
				error = udpSocket->writeDatagram(sendbuf[player].get(), address, ctrl->remotePlayerPort.get(player));
				if (error < 0) qWarning() << "StreamManager Loop: Cannot send ping to player" << player << udpSocket->errorString() 
										  << "This is normal when trying to reach an IPv6 from an IPv4 and goes away when the remote host initiates the connection.";
			}
			// Data packet is sent, clear ist for the next transfer. This allows to fill it during demuxing of received packets
			sendbuf[player].clear(ctrl->packetHeader.get());
		}
	}

	// The data is already in the proper buffers, now deliver it!
	// Decode the voice data
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		// If player is not active or buffer is still in prefill generate zeros
		if (idletime[p] == IDLETIME ||  remoteVoiceFifo->isPrefilling(p)) memset((void*)remotevoice[p], 0, FRAMESIZE * sizeof(Mono) * BUNDLE);
		else {
			// Get remote voice stream from buffer
			remotevoiceenc = remoteVoiceFifo->get(p);
			//qDebug() << remotevoiceenc.toHex();
			if (remotevoiceenc.isNull()) {
				//qDebug() << "Missing content";
				error = opus_decode_float(voiceDecoder[p], (unsigned char *) NULL, 0, remotevoice[p], FRAMESIZE * BUNDLE, 1);
				if (error < 0) qWarning("StreamManager: Cannot decode remote voice player %d, error: %d", p, error);
			}
			else {
				error = opus_decode_float(voiceDecoder[p], (unsigned char *) remotevoiceenc.data(), remotevoiceenc.length(), remotevoice[p], FRAMESIZE * BUNDLE, 0);
				if (error < 0) qWarning("StreamManager: Cannot decode remote voice player %d, error: %d", p, error);
			}
		}
	}

	// The gamemaster streams the sound, the players receive it. 
	if ( ctrl->mode.get() != MODE_MASTER )	{
		// Try to get remote ambient stream from buffer
		remoteambientenc = remoteAmbientFifo->get(0);
		if (remoteambientenc.isNull()) {
			// No data available! See what the decoder can do!
			if (remoteAmbientFifo->isPrefilling(0)) memset((void*)ambient, 0, FRAMESIZE * sizeof(Stereo_16) * BUNDLE);
			else {
				error = opus_decode(ambientDecoder, (unsigned char *) NULL, 0, (opus_int16*)ambient, FRAMESIZE * BUNDLE, 1);
				if (error < 0) qWarning("StreamManager: Cannot decode remote ambient, error: %d", error);
				//qWarning() << "StreamManager: Missing ambient content";
			}
		} else {
			//qDebug() << "Ambient Packet length:" << remoteambientenc.length();
			error = opus_decode(ambientDecoder, (unsigned char *) remoteambientenc.data(), remoteambientenc.length(), (opus_int16*)ambient, FRAMESIZE * BUNDLE, 0);
			if (error < 0) qWarning("StreamManager: Cannot decode remote ambient, error: %d", error);
		}
	} else if (!ambientPlaying) {
		// If we are master, the decoded data is already waiting in "ambient" so nothing is to do then unless we do not stream ourselves
		// IN this case we have to provide silence in case the buffer still contains some garbage
		memset((void*)ambient, 0, FRAMESIZE * sizeof(Stereo_16) * BUNDLE);
		// FIXME: Do this only when we change von straming to non-streaming to save CPU
	}
	framessent++;
	if (framessent >= LOGSIZE) {
		emit(ctrl->statistics(rxpackets, txbytes, rxbytes)); 	
		for (int i=0; i<MAXPLAYERLINKS; i++) {
			rxpackets[i] = 0;
			txbytes[i] = 0;
			rxbytes[i] = 0;
		}
		framessent = 0;
	}
	gettimeofday(&actualtime, NULL);
	delta = actualtime.tv_usec - starttime.tv_usec;
	if (delta < 0) delta +=1000000;
	if (delta > 40000) qDebug("Streammanager: Whole loop took %d µs", delta);

}

void StreamManager::demux(QByteArray recbuf, QHostAddress senderAddress, uint16_t senderPort)
{
	//qDebug() << "Got Packet from" << senderAddress.toString() << ":" << senderPort;
	int player = -1;
	uint16_t length;
	bool isSendingVoice;
	int error;
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		if (senderAddress.isEqual(ctrl->remotePlayerAddress.get(p))) player = p; // Found the IP address in the player list
	}
	//qDebug() << "StreamManager Demux: Got packet from sender:" << senderAddress.toString();
	// Check for header
	if (!recbuf.startsWith(ctrl->packetHeader.get()))	{
		// Packet cannot identify itself properly - discard it
		qWarning() << "StreamManager Demux: Received UDP packet with wrong passphrase. Ignoring it!"; 
		return;
	} 
	recbuf.remove(0, ctrl->packetHeader.get().length());
	//if (recbuf.size()>1 && (uint8_t)recbuf.at(0) == PACKET_PING) qDebug("StreamManager: Ping packet received");

	
	if (player == -1) {
		qWarning() << "StreamManager Demux: Got packet from unknown sender:" << senderAddress.toString() << "Could there be some routing in between? Check!";
		if (recbuf.size()>1 && (uint8_t)recbuf.at(0) == PACKET_PING) {
			length = (unsigned char)recbuf.at(1);
			if (length+2 > recbuf.size()) return;
			QByteArray name = recbuf.mid(2, length);
			for (int p=0; p<MAXPLAYERLINKS; p++) {
				if (name == ctrl->remotePlayerName.get(p).toUtf8()) {
					qDebug("StreamManager: Ping packet from unknown sender has valid passphrase and name - adjusting remote player's address and port");
					ctrl->remoteAddressOverride.set(true, p);
					ctrl->remotePlayerAddress.set(senderAddress, p);
					player = p;
				}
			}
		} else qDebug("StreamManager: Cannot use other packets than PING to adjust remote players address");
	}
	if (player == -1) return; // Sender is still unknown! Ignore!
	
	// Well, at least the remote partner seems to exist. Zero the idle counter and go to status INIT. 
	idletime[player] = 0;
	if (ctrl->connectionStatus.get(player) == CON_DISCONNECTED) {
		ctrl->connectionStatus.set(CON_INIT, player); 
		emit(ctrl->connectionStateChange(player));
	}

	// Now let's chop the buffer into the different data sniplets it may contain
	//qDebug("Demuxing packet:");
	//qDebug() << recbuf.toHex();
	isSendingVoice = false;
	rxpackets[player]++;
	rxbytes[player] += recbuf.size();
	while (recbuf.size() > 0) {
		// When the remote host starts transmitting the stream or answered our ping with a pong, we can start transmitting data packets too. The connection is stable!
		if ((uint8_t)recbuf.at(0) != PACKET_PING && ctrl->connectionStatus.get(player) != CON_READY) {
			ctrl->connectionStatus.set(CON_READY, player);
			emit(ctrl->connectionStateChange(player));
		}
		//qDebug() << buffer.mid(0, 5).toHex();
		//qDebug() << "Length of received packet" << length;
		// Byte 0 defines the type
		switch ((uint8_t)recbuf.at(0)) {
			case PACKET_VOICE :
				if (recbuf.size() < 5) qWarning("StreamManager Demux: Crippled voice packet detected");
				else {
					length = (unsigned char)recbuf.at(1) * 256 + (unsigned char)recbuf.at(2); // Length of the packet
					//qDebug("    Voice packet with length %d detected", length);
					// A voice data packet requires a voice data fifo
					remoteVoiceFifo->append(player, recbuf.mid(3,length+2)); // length is only the length of the data, add the whole buffer
					recbuf.remove(0, length+5);
					isSendingVoice = true;
				}
			break;
			case PACKET_AMBIENT:
				if (recbuf.size() < 5) qWarning("StreamManager Demux: Crippled ambient packet detected");
				else {
					// FIXME: Make sure that only one streaming server is accepted
					length = (unsigned char)recbuf.at(1) * 256 + (unsigned char)recbuf.at(2); // Length of the packet
					//qDebug("    Ambient packet with length %d detected", length);
					if (ctrl->mode.get() == MODE_MASTER) qWarning("StreamManager Demux: Received audio frame while beeing master - ignoring it! Are you running a local loopback without the LOOPBACK option?");
					else remoteAmbientFifo->append(0, recbuf.mid(3,length+2)); // length is only the length of the data, add the whole buffer
					recbuf.remove(0, length+5);
				}
			break;
			case PACKET_PING:
				//qDebug("    Idle ping received!");
				if (senderPort != ctrl->remotePlayerPort.get(player)) { 
					qDebug() << "StreamManager Demux: Got packet from weird port number:" << senderPort << "Probably there's a firewall that remaps port numbers. Using the new port from now on.";
					ctrl->remotePlayerPort.set(senderPort, player);
				}
				if (recbuf.size() < 1) {qWarning("Streammanager: Crippled ping packet received"); return;}
				length = (unsigned char)recbuf.at(1);
				if (length+2 > recbuf.size()) {qDebug("StreamManager: Crippled ping packet detected"); return;}
				// Answer with a PONG packet immediately - connection is not up yet, so it cannot be used
				sendbuf[player].clear(ctrl->packetHeader.get());
				sendbuf[player].append(QByteArray(1, PACKET_PONG));
				error = udpSocket->writeDatagram(sendbuf[player].get(), ctrl->remotePlayerAddress.get(player), ctrl->remotePlayerPort.get(player));
				if (error < 0) qWarning() << "StreamManager Loop: Cannot send pong to player" << player << udpSocket->errorString();
				return; // Ping packet cannot be combined with data - sorry! No further processing required!
			break;
			case PACKET_PONG:
				qDebug("StreamManager: Pong packet received");
				return; // Pong packets cannot be combined with data - sorry! No further processing required!
			break;
			case 0 ... MAXBULKBAYS-1:
				if (recbuf.size() < 5) qWarning("StreamManager Demux: Crippled data packet detected");
				else {
					// Got a sniplet
					// Acknowledge the sniplet
					//qDebug("StreamManager Demux: Got a sniplet from player %d for bay %d -> Acknowledging it", player, recbuf.at(0));
					char ack[4];
					ack[0] = char(uint8_t(recbuf.at(0))+MAXBULKBAYS); // The bay of the sniplet
					ack[1] = recbuf.at(2); // The number of the sniplet, 24 bits
					ack[2] = recbuf.at(3);
					ack[3] = recbuf.at(4);
					sendbuf[player].append(QByteArray(ack, 4));
					
					bulkbay->addToRxBuffer(player, recbuf.mid(0, int((uint8_t)recbuf.at(1))+8));
					recbuf.remove(0,int((uint8_t)recbuf.at(1))+8);
				}
			break;
			case MAXBULKBAYS ... MAXBULKBAYS * 2 - 1:
				if (recbuf.size() < 4) qWarning("StreamManager Demux: Crippled acknowledge packet detected");
				else {
					// Got acknowledgement for one of our sniplets sent recently 
					//qDebug("StreamManager Demux: Got an acknowledgement from player %d for bay %d", player, recbuf.at(0) - MAXBULKBAYS);
					bulkbay->acknowledgeSniplet(player, uint32_t(recbuf.at(0)) - MAXBULKBAYS, 
						(uint32_t(uint8_t(recbuf.at(1)))<<16) + (uint32_t(uint8_t(recbuf.at(2)))<<8) + uint32_t(uint8_t(recbuf.at(3))));
					recbuf.remove(0,4); // Sniplet acknowledgement has always 4 bytes
				}
			break;
			default:
				qWarning() << "StreamManager Demux: Packet received with ID not yet implemented:" << (uint32_t)(unsigned char)recbuf.at(0);
				recbuf.clear();
		}
	}
	ctrl->isSendingVoice.set(isSendingVoice, player);
	PACKETDEBUG(qDebug("---- Demux done ----");)
}

bool StreamManager::startMusicPlayer()
{
	if (ctrl->musicList.get().size() == 0) return false;  // Nothing to play? Refuse to do so!
	if (musicPlayer->state() != QProcess::NotRunning) {qWarning("StreamManager: Music player process is already running!"); return false;} // Should not be required!

	musicPlayer->setWorkingDirectory(ctrl->musicDir.get().absolutePath());

	QString program = "mpg123";
	QStringList arguments;
	arguments << "-s" << "-r" << "48000" << "--stereo" << "--smooth" << "-o" << "raw" << "-e" << "s16" << "-z" << "--gapless";
	arguments << ctrl->musicList.get().at(ctrl->musicPos.get());
	//qDebug() << "StreamManager: Starting mpg123 for music" << program << arguments;
	musicPlayer->start(program, arguments, QIODevice::ReadWrite);
	//qDebug() << "StreamManager: musicPlayerState:" << musicPlayer->state() << "error:" << musicPlayer->error()
	//		 << "stderr:" << musicPlayer->readAllStandardError();
	return true;
}

bool StreamManager::startSoundPlayer()
{
	if (ctrl->soundFile.get() == "") return false; // Nothing to play? Refuse to try!
	if (soundPlayer->state() != QProcess::NotRunning) {qWarning("StreamManager: Sound player process is already running!"); return false;} // Should not be required!

	soundPlayer->setWorkingDirectory(ctrl->dir[DIR_SOUND].absolutePath());

	QString program = "mpg123";
	QStringList arguments;
	arguments << "-s" << "-r" << "48000" << "--stereo" << "--smooth" << "-o" << "raw" << "-e" << "s16" << "-z" << "--gapless";
	arguments << ctrl->soundFile.get();
	//qDebug() << "StreamManager: Starting mpg123 for sound" << program << arguments;
	soundPlayer->start(program, arguments, QIODevice::ReadWrite);
	//qDebug() << "StreamManager: soundPlayerState:" << soundPlayer->state() << "error:" << soundPlayer->error()
	//		 << "stderr:" << soundPlayer->readAllStandardError();
	return true;
}
