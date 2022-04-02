#ifndef CONFIG_H
#define CONFIG_H

#define MAXPLAYERLINKS 5
#define MAXPLAYERS (MAXPLAYERLINKS+1)
#define LOCALPLAYER MAXPLAYERLINKS
#define AMBIENTLEFT (LOCALPLAYER+1)
#define AMBIENTRIGHT (LOCALPLAYER+2)

#define MAXVIEWERLINKS 1
#define MAXNPCS 10
#define MAXDIEVARIANTS 4
#define MAXDATAGRAMSIZE 65000
#define MAXPACKETSIZE 400

#define PACKETHEADER "GTG$$01#"
#define BASEDIR "ArthursTable"

#define PORTNUMBER 37250 // UDP port for communication

#define VOICEBUFFERS 3
#define IDLETIME 200
#define PINGTIME 100

#define SAMPLERATE 48000l // Don't mess with this setting! Opus only accepts 48k for high quality, no 44k1 ! 
#define FRAMESIZEMS 10l // ms, only values according to Opus codec's possibilities are allowed, notice that more than one frame may be bundeled
#define FRAMESIZE (SAMPLERATE * FRAMESIZEMS / 1000l)
#define WINDOWSIZE 1024 // Windowsize is the next larger power of 2 to the double framesize

#define BUNDLE 2l // We need very low latency to allow for local voice echo but can afford bundling some frames prior to transmission

#define VOICEBITRATE 24000l
#define VOICEBYTESPERFRAME (VOICEBITRATE * FRAMESIZEMS / 8000l)
#define VOICEFIFOLENGTH 20 
// FIXME: Try to get this value dynamically adjustable, 

#define AMBIENTBITRATE 96000l
#define AMBIENTBYTESPERFRAME (AMBIENTBITRATE * FRAMESIZEMS / 8000l)
#define AMBIENTFIFOLENGTH 50

#define MONO 1
#define STEREO 2

#define LEFT 0
#define RIGHT 1

#define AUTOMUTEDELAY (100 / FRAMESIZEMS)

#define LEVELBALANCE 200

#define DELAYLINELENGTHMS 2000l
#define DELAYLINELENGTH (DELAYLINELENGTHMS * SAMPLERATE / 1000l)

#define HEADDELAY 632e-6 // Size of a standard head divided by the speed of sound

#define MAXSTREAMS (MAXPLAYERLINKS + MONO + STEREO) // Playerlinks + localplayer + ambient

#define MININFOPANELWIDTH 400
#define CONTROLPANELWIDTH 200
#define MASTERPANELWIDTH 200

#define MAXBULKBAYS 64 // 64 max!
#define BUF_LOW_PRIO (MAXBULKBAYS/4)
#define BUF_MID_PRIO (MAXBULKBAYS/2)
#define BUF_HIGH_PRIO (MAXBULKBAYS*3/4)

#define SNIPLETSIZE 64

#define RXBLANKTIME 250 //ms
#define TXBLANKTIME 500 // ms

#define VOICEPRESELECTIONS 8


#define THUMBNAILSIZE 110

#define MAXTEAMS 3 // From a guy with 30 years of gamemaster experience: Do not try to handle more than two groups at the same time!

#define LOGSIZE 50 // FIXME: What's that for?

#define MAXMARKERS 100 // Not more than 255 to fit in a char - do not use 256!

#endif