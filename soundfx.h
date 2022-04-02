#ifndef SOUNDFX_H
#define SOUNDFX_H

#include <QThread>
#include "config.h"
#include "control.h"
#include "types.h"

enum WindowFunctions {WINDOW_FLAT=0, WINDOW_COSINE, WINDOW_OGG};

#define INPUTBUFFERSIZE SAMPLERATE * 1

class LowPass
{
public:
	LowPass();
	void setCornerFrequency(float freq);
	Mono filt(Mono);
private:
	Mono m;
	float tau;
};

class SoundFx
{
public:
	SoundFx();
	void run(Mono *, Mono **, Stereo_16 *, Stereo *, int);
	static QStringList getEffectNames();
	static QStringList getComEffectNames();
	void applyComEffect(Mono * in, Mono * out, ComEffect effect, int size);
protected:
	void pushToInputBuffer(Mono *data);
	void addToInputBuffer(Mono *data);
	//void getFromInputBuffer(Mono * data, int window);
	void getFromOutputBuffer(Mono * data);
	void addToOutputBuffer(Mono * data, float amplitude);
	//void patchToOutputBuffer(Mono * data, int length, int offset, float amplitude);
	void getFromInputBuffer(Mono * data, int window, float delay, float scale, float amplitude);
	void patchToOutputBuffer(Mono * data);
	void strech(float * datain, float * dataout, int oldsize, int newsize);
private:
	void applyVoiceEffect(Mono * data);
	Mono getDelayed(int stream, int index, float delay);
	Stereo getWithDirectivity(int stream, int index);
	void initDirectivity(float angle[MAXSTREAMS]);
	Stereo limiter(Stereo s);
	Mono_16 * localvoicecp;
	Mono * delayline[MAXSTREAMS];
	int delaylinepos;
	float mutegain;
	int mutecount;
	float damper;
	LowPass * lp_l[MAXSTREAMS], * lp_r[MAXSTREAMS];
	float delay_l[MAXSTREAMS], delay_r[MAXSTREAMS];
	float lowpass_l[MAXSTREAMS], lowpass_r[MAXSTREAMS];
	Mono * inbuf, * outbuf;
	int inbufpos, outbufpos;
	int outbuffill;
	float *windowfunc[3];
	Mono * frame1, * frame2, * frame3;
	int offset;
	VocEffect oldEngine;
};

#endif