#ifndef TYPES_H
#define TYPES_H
#include <QString>
#include "config.h"
#include "tools.h"

enum Mode {MODE_NONE, MODE_PLAYER, MODE_MASTER, MODE_VIEWER, MODE_LOOPBACK};
// 0...MAXBULKBAYS-1 is reserved for bulk bays, MAXBULKBAYS ... 2xMAXBULKBAYS-1 is reserved for the acknowledgements
enum Packettype {PACKET_VOICE = 2 * MAXBULKBAYS, PACKET_AMBIENT, PACKET_PING, PACKET_PONG}; 
enum Remotes {REMOTE_PLAYERVOICEEFFECT, REMOTE_DOMINANTAMBIENT, REMOTE_COMMATRIX, REMOTE_NPCVISIBILITY, REMOTE_MAPVISIBILITY, 
			REMOTE_COMEFFECT, REMOTE_TEAM};
enum Dirs {DIR_WORKING, DIR_NPCS, DIR_PICS, DIR_CONFIG, DIR_MUSIC, DIR_SOUND, DIR_SKETCHES, DIR_SHARE, NDIRS};
enum Focus {FOCUS_NPC, FOCUS_MAP, FOCUS_SLIDE, FOCUS_DUNGEON};
enum ComState {COM_NONE, COM_PARTIAL, COM_FULL};
enum ConStatus {CON_DISCONNECTED, CON_INIT, CON_READY};
enum TrackerType {TRACKER_NONE, TRACKER_BNO055};
enum ComEffect {COMEFFECT_NONE, COMEFFECT_MUTE, COMEFFECT_FAR, COMEFFECT_RADIO, COMEFFECT_PHONE};
enum VocEffect {VOICEEFFECT_NONE, VOICEEFFECT_PITCH, VOICEEFFECT_SWARM, VOICEEFFECTS};

enum TextFormat {FMT_PLAYER_0, FMT_PLAYER_1, FMT_PLAYER_2, FMT_PLAYER_3, FMT_PLAYER_4, FMT_PLAYER_5, 
				 FMT_PLAYER_6, FMT_PLAYER_7, FMT_LOCAL, FMT_WARNING, FMT_ERROR};
#define FMT_BOT true
#define FMT_HUMAN false

typedef float Mono;

typedef int16_t Mono_16;



class Stereo {
public:
	float l, r;
	Stereo() {
		this->l = 0.0;
		this->r = 0.0;
	}
	Stereo(float left, float right) {
		this->l = left;
		this->r = right;
	}
	Stereo operator+(const Stereo& s) {
		Stereo r;
		r.l = this->l+s.l;
		r.r = this->r+s.r;
		return r;
	}
	Stereo operator*(const float& s) {
		Stereo r;
		r.l = this->l * s;
		r.r = this->r * s;
		return r;
	}
	void operator+=(const Stereo& s) {
		this->l += s.l;
		this->r += s.r;
	}
	void operator*=(const float& s) {
		this->l *= s;
		this->r *= s;
	}
	void operator=(const Mono& i) {
		this->l = i;
		this->r = i;
	}
	void operator=(const Mono_16& i){
		this->l = (float)i / 32768.0f;
		this->r = (float)i / 32768.0f;
	}
	bool operator!=(const Stereo& s) {
		return !(this->l == s.l && this->r == s.r);
	}
	bool operator==(const Stereo& s) {
		return this->l == s.l && this->r == s.r;
	}
};

class Stereo_16 {
public:
	int16_t l, r;
	Stereo_16() {this->l=0; this->r=0;}
	void operator *=(const float a) {
		this->l = int16_t((float)this->l * a);
		this->r = int16_t((float)this->r * a);
	}
	Stereo_16 operator +(const Stereo_16 a) {
		Stereo_16 b;
		b.l = limit((int32_t)this->l + (int32_t)a.l, -32768, 32767);
		b.r = limit((int32_t)this->r + (int32_t)a.r, -32768, 32767);
		return b;
	}
	void operator +=(const Stereo_16 a) {
		this->l = limit((int32_t)this->l + (int32_t)a.l, -32768, 32767);
		this->r = limit((int32_t)this->r + (int32_t)a.r, -32768, 32767);
	}
	Stereo_16 operator *(const float a) {
		Stereo_16 b;
		b.l = (int16_t)limit((float)this->l * a, -32768.0f, 32768.0f);
		b.r = (int16_t)limit((float)this->r * a, -32768.0f, 32768.0f);
		return b;
	}

};

class FileInfo {
public:
	uint8_t dirnr;
	QString name;
	QByteArray hash;
	FileInfo() {}
	FileInfo(uint8_t d, QString n, QByteArray h) {this->dirnr = d; this->name = n; this->hash = h;}
	bool operator==(const FileInfo & f) const {return (this->dirnr == f.dirnr) && (this->name.toUtf8() == f.name.toUtf8()) && (this->hash == f.hash);}
};

typedef struct {
		QString name;
		uint8_t dirnr;
		QByteArray data;
} WholeFile;


#endif