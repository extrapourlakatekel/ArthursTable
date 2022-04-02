#include <assert.h>
#include "soundfx.h"


SoundFx::SoundFx()
{
	localvoicecp = new Mono_16[FRAMESIZE];
	for (int p=0; p<MAXSTREAMS; p++) {
		delayline[p] = new Mono[DELAYLINELENGTH];
		memset(delayline[p], 0, DELAYLINELENGTH*sizeof(MONO));
		lp_l[p] = new LowPass(); 
		lp_l[p]->setCornerFrequency(1000.0);
		lp_r[p] = new LowPass(); 
		lp_r[p]->setCornerFrequency(1000.0);
	}
	//inbuf = new Mono[FRAMESIZE*2]; // We need 2 x FRAMESIZE buffer entries to collect enough for one window
	inbuf = new Mono[INPUTBUFFERSIZE];
	//for (int i=0; i<FRAMESIZE*2; i++) inbuf[i] = 0.0; // Do we need this?
	for (int i=0; i<INPUTBUFFERSIZE; i++) inbuf[i] = 0.0;
	outbuf = new Mono[FRAMESIZE*6]; // The output might need more during the downpitch
	for (int i=0; i<FRAMESIZE*6; i++) outbuf[i] = 0.0; // Do we need this?
	inbufpos = 0;
	outbufpos = 0;
	outbuffill = 0;
	windowfunc[WINDOW_FLAT] = new float[2*FRAMESIZE];
	windowfunc[WINDOW_COSINE] = new float[2*FRAMESIZE];
	windowfunc[WINDOW_OGG] = new float[2*FRAMESIZE];
	for (int i=0; i<2*FRAMESIZE; i++) {
		windowfunc[WINDOW_FLAT][i] = 1.0f;
		windowfunc[WINDOW_COSINE][i] = (1.0f-cosf((float)i * 2.0f * M_PI / (float)(2*FRAMESIZE)))/2.0f;
		windowfunc[WINDOW_OGG][i] = sinf(0.5f * M_PI * pow(sinf(((float)i + 0.5f) / (float)(2*FRAMESIZE) * M_PI), 2.0f));
	}		
	frame1 = new Mono[2*FRAMESIZE];
	frame2 = new Mono[4*FRAMESIZE];
	frame3 = new Mono[2*FRAMESIZE];
	offset = 0;
	oldEngine = VOICEEFFECT_NONE;
}

Stereo SoundFx::limiter(Stereo s)
{
	float peak = fmax(fabs(s.l), fabs(s.r));
	if (peak * damper > 1.0f) damper *= 1.0f / (peak * damper); 
	else damper += (1.0f - damper) / float(SAMPLERATE);
	s *= damper;
	return s;
}

void SoundFx::run(Mono * lv, Mono ** rv, Stereo_16 * amb, Stereo * lso, int set)
{
	//qDebug("SoundFX: processing");
	assert(lv != NULL);
	assert(rv != NULL);
	assert(amb != NULL);
	assert(lso != NULL);
	assert(set >= 0);
	assert(set < BUNDLE);
	Mono * localvoice;
	Mono * remotevoice[MAXPLAYERLINKS];
	Stereo_16 * ambient;
	Stereo * localsoundout;

	// Make the pointer point to the proper segment of the input data depending on the set we are working on
	localvoice = &lv[FRAMESIZE * set];
	for (int p=0; p<MAXPLAYERLINKS; p++) {assert(&rv[p] != NULL); remotevoice[p] = &rv[p][FRAMESIZE * set];}
	ambient = &amb[FRAMESIZE * set];
	localsoundout = &lso[FRAMESIZE * set];
	float localechovolume; // Store locally to speed things up
	if (ctrl->localEchoVolume.get() == 0) localechovolume = 0.0;
	else localechovolume = powf(10.0, ctrl->localEchoVolume.get()/100.0*2.0-2.0); 
	const float angle[MAXPLAYERS+STEREO] = {0.0, -60.0, 60.0, -30.0, 30.0, 0.0, -45.0, 45.0};
	float volume[MAXPLAYERS+STEREO];
	
	float delta[MAXPLAYERS+STEREO];
	
	for (int p=0; p<MAXPLAYERLINKS; p++) volume[p] = powf(10.0, ctrl->remotePlayerVolume.get(p)/100.0*3.0-2); // Fills the first MAXPLAYERLINKS entries with the proper volumes between 0.1 and 10 in log scale
	volume[AMBIENTLEFT] = volume[AMBIENTRIGHT] = powf(10.0, ctrl->ambientVolume.get()/100.0*2.0-3.0)/32768.0; // Volume of ambient can be set from 0.0 to 1.0 with a scaling factor of 2^15

	//Calculate energy levels of streams for activity detection and peak detection for vu meter
	float energy[MAXPLAYERS];
	float peak[MAXPLAYERS];
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		energy[p] = 0.0;
		peak[p] = 0.0;
		for (int i=0; i<FRAMESIZE; i++) {
			energy[p] += remotevoice[p][i] * remotevoice[p][i];
			peak[p] = fmaxf(peak[p], fabsf(remotevoice[p][i]));
		}
	}
	//qDebug("SoundFx: Calculating energy");
	energy[LOCALPLAYER] = 0.0;
	peak[LOCALPLAYER] = 0.0;
	for (int i=0; i<FRAMESIZE; i++) {
		energy[LOCALPLAYER] += localvoice[i] * localvoice[i];
		peak[LOCALPLAYER] = fmaxf(fabsf(localvoice[i]), peak[LOCALPLAYER]);
	}
	// Automute if local player is silent
	// FIXME: Something bad happens betwee here...
	bool talking;
	if (mutecount < AUTOMUTEDELAY) {mutecount ++; talking = true;} else talking = false;
	//qDebug("SoundFX: Mutecount is: %d", mutecount);
	if (energy[LOCALPLAYER]/FRAMESIZE > powf(10.0, (float)ctrl->inputSensitivity.get()/30.0)/200000.0) mutecount = 0; 
	if (ctrl->inputSensitivity.get() == 0) mutecount = 0; // When sensitivity is at minimum, always be on
	for (int i=0; i<FRAMESIZE; i++) {
		if (talking) mutegain += (1.0 - mutegain) * 0.01; // Do an exponential in- or decrease to avoid clicks
		else mutegain += (0.0 - mutegain) * 0.001;
		localvoice[i] *= mutegain; 
	}
	// ... And here
	ctrl->vuLocalVoice.set(peak[LOCALPLAYER]); // Tell Control to show the proper VU level
	ctrl->localVoiceActive.set(talking);
	
	//qDebug("SoundFX: Applying voice effects");
	// Process voice effects for local player 
	applyVoiceEffect(localvoice);

	//qDebug("SoundFX: Copying buffers");
	for (int i=0; i<FRAMESIZE; i++) {
		delaylinepos = (delaylinepos + 1) % DELAYLINELENGTH;
		// Add localsound to buffer without affecting the volume. Only needed for room acoustics
		delayline[MAXPLAYERS][delaylinepos] = Mono(localvoice[i]);
		// Add all the players to the delay line after voice effect processing and volume control
		for (int p=0; p<MAXPLAYERLINKS; p++) delayline[p][delaylinepos] = remotevoice[p][i] * volume[p];
		// Add ambient sound to buffer - required for vital stereo impession
		delayline[AMBIENTLEFT][delaylinepos] = (Mono)ambient[i].l * volume[AMBIENTLEFT];
		delayline[AMBIENTRIGHT][delaylinepos] = (Mono)ambient[i].r * volume[AMBIENTRIGHT];
	}

	//Calculate angle differences between the heading of the local player and the direction to the sound source
	//qDebug("Calculating angles to sound sources");
	for (int p=0; p<MAXPLAYERS+STEREO; p++) delta[p] = angle[p] - ctrl->heading.get();
	
	// Configure the delays and the lowpass filters - done only one and static for one frame to save CPU
	initDirectivity(delta);
	float peakl=0;
	float peakr=0;
	//qDebug("Mixing streams");
	// Now mix all!
	Stereo temp;
	for (int i=0; i<FRAMESIZE; i++) {
		
		// Add local echo if desired
		temp = localvoice[i] * localechovolume; 
		
		// Add the voices from the other players
		for (int p=0; p<MAXPLAYERLINKS; p++) temp += getWithDirectivity(p, i);
		
		// Add the ambient sounds - both channels to both ears each!
		temp += getWithDirectivity(AMBIENTLEFT, i);
		temp += getWithDirectivity(AMBIENTRIGHT, i);

		// Caclulate the peak values for VU meter output
		peakl = fmaxf(fabsf(temp.l), peakl);
		peakr = fmaxf(fabsf(temp.r), peakr);
		
		// So a soft limiting to squeeze it into 16 bit audio
		localsoundout[i] = limiter(temp);
	}
	//qDebug("Done mixing streams");
	ctrl->vuOut.set(Stereo(peakl, peakr));
}

void SoundFx::pushToInputBuffer(Mono *data)
{
	//memcpy(&inbuf[inbufpos * FRAMESIZE], data, FRAMESIZE*sizeof(Mono)); 
	//inbufpos = (inbufpos + 1) % 2;
}

void SoundFx::addToInputBuffer(Mono *data)
{
	for (int i=0; i<FRAMESIZE; i++) {
		inbuf[inbufpos] = data[i]; 
		inbufpos = (inbufpos + 1) % INPUTBUFFERSIZE;
	}
}

void SoundFx::getFromInputBuffer(Mono * data, int window, float delay, float scale, float amplitude)
// Fills a 2*FRAMESIZE long array with data from the buffer
{
	if (delay * SAMPLERATE + scale * 2.0 * FRAMESIZE > INPUTBUFFERSIZE) qWarning("SoundFx: Tried to get a too old sample from the input buffer");
	int shift = (int)(2.0f * (float)FRAMESIZE * scale) + delay * (float)SAMPLERATE;
	float dc = 0.0;
	float integral = 0.0;
	for (int i=0; i<2*FRAMESIZE; i++) {
		data[i] = inbuf[(int)(inbufpos + (int)((float)i*scale)  - shift + INPUTBUFFERSIZE) % INPUTBUFFERSIZE] * windowfunc[window][i] * amplitude; // FIXME: Implement at least linear sample interpolation instead of stupid sample picking!
		integral += windowfunc[window][i] * amplitude;
		dc += data[i];
	}
	dc /= integral;
	// Remove DC component
	/*
	for (int i=0; i<2*FRAMESIZE; i++) {
		data[i] -= dc * windowfunc[window][i];
	}*/
}

/*
void SoundFx::patchToOutputBuffer(Mono * data, int length, int offset, float amplitude)
// Expects a length long frame and adds it to the outputbuffer
{
	assert (length <= 4 * FRAMESIZE);
	// Patch sniplets to the buffet 
	for (int i=0; i<length; i++) {
		outbuf[(i + FRAMESIZE * outbufpos + offset) % (FRAMESIZE * 6)] += data[i] * amplitude; //
	}
}*/

void SoundFx::patchToOutputBuffer(Mono * data)
// Expects a length long frame and adds it to the outputbuffer
{
	// Patch sniplet to the output buffet 
	for (int i=0; i<FRAMESIZE * 2; i++) {
		outbuf[(i + FRAMESIZE * outbufpos) % (FRAMESIZE * 6)] += data[i];
	}
}

void SoundFx::addToOutputBuffer(Mono * data, float amplitude)
// Expects a length long frame and adds it to the outputbuffer
{
	// Patch sniplets to the buffet until we have enough to send one FRAMESIZE frame
	for (int i=0; i<2*FRAMESIZE; i++) {
		outbuf[(i + FRAMESIZE * outbufpos) % (FRAMESIZE * 6)] += data[i] * amplitude; //
	}
}


void SoundFx::getFromOutputBuffer(Mono * data)
// Copes a FRAMESIZE long segment of the output buffer to *data and sets the segment to zero. This is requires since only adding to the output buffer is done
{
	memcpy(data, &outbuf[outbufpos * FRAMESIZE], FRAMESIZE * sizeof(Mono)); 
	memset(&outbuf[outbufpos * FRAMESIZE], 0, FRAMESIZE * sizeof(Mono));
	outbufpos = (outbufpos+1) % 6;
	outbuffill -= FRAMESIZE;
	//memcpy(data, &inbuf[(inbufpos - 2 + 4) % 4 * FRAMESIZE], FRAMESIZE * sizeof(Mono)); DEBUG only!
}

void SoundFx::strech(float * datain, float * dataout, int oldsize, int newsize)
{
	for (int i=0; i<newsize; i++) {
		//FIXME: Do interpolation, linear oder sinc!
		float pos = (float)i*float(oldsize)/(float)newsize;
		float rightpart = pos - floorf(pos);
		float leftpart = 1.0 - rightpart;
		if (i<newsize-1) dataout[i] = datain[(int)pos] * leftpart + datain[(int)pos+1] * rightpart;
		else dataout[i] = datain[(int)pos] * leftpart;
	}
}

void SoundFx::applyVoiceEffect(Mono * data)
{
	bool reset;
	// Make local copies of the effect number and the effect parameters to avoid changes during procession
	VocEffect effectEngine;
	QByteArray effectConfig;
	int voices;
	int effectSelected = ctrl->voiceEffectSelected.get();
	assert(effectSelected >= -1);
	assert(effectSelected < 8);
	if (effectSelected == -1) effectEngine = VOICEEFFECT_NONE;
	else {
		effectConfig = ctrl->voiceEffectConfig.get(ctrl->voiceEffectSelected.get());
		if (effectConfig.isEmpty()) effectEngine = VOICEEFFECT_NONE;
		else effectEngine = (VocEffect)effectConfig.at(0);
		// Shove the actual data into the input buffer (same for all effects)
		reset = effectEngine != oldEngine;
		if (reset) oldEngine = effectEngine;
	}
	addToInputBuffer(data);
	//int newsize;
	switch (effectEngine) {
		// Now interprete the effect!
		case VOICEEFFECT_NONE:
			//qDebug("SoundFx: Applying No Effect");
			// Simply pass through, but already use the window functionality to allow for seamless effect transition
			//getFromInputBuffer(frame1, WINDOW_COSINE);
			//addToOutputBuffer(frame1, 1.0f);
			getFromInputBuffer(frame1, WINDOW_COSINE, 0.0, 1.0, 1.0);
			patchToOutputBuffer(frame1);
		break;
		case VOICEEFFECT_PITCH:
			if (effectConfig.size() != 2) {qWarning("SoundFX: Error in pitch configuration"); break;}
			if (reset) offset = 0;
			//qDebug("SoundFx: Applying Pitch Effect");
			//getFromInputBuffer(frame1, WINDOW_COSINE);
			getFromInputBuffer(frame1, WINDOW_COSINE, 0.0, powf(2.0f, (float)effectConfig.at(1)/12.0f), 1.0);
			//newsize = (int)((float)(FRAMESIZE*2)*powf(2.0f, -(float)effectConfig.at(1)/12.0f));
			//assert(newsize <= 4*FRAMESIZE);
			//assert(newsize >= FRAMESIZE);
			//strech(frame1, frame2, 2*FRAMESIZE, newsize);
			//while (offset < FRAMESIZE) {patchToOutputBuffer(frame2, newsize, offset, 1.0f); offset += newsize/2;}
			//offset -= FRAMESIZE;
			patchToOutputBuffer(frame1);
			offset += FRAMESIZE;
			//addToOutputBuffer(frame1, 0.5);
		break;
		case VOICEEFFECT_SWARM:
			if (effectConfig.size() != 156) {qWarning("SoundFX: Error in swarm configuration"); break;}
			voices = effectConfig.at(1);
			assert (voices >= 2);
			assert (voices <= 50);
			for (int i=0; i<voices; i++) {
				// Entry 2 contains the basic pitch in halftones and entry 3 contains the variation in quartertones
				float pitch = (float)effectConfig.at(2) + (float)effectConfig.at(3) * (float)effectConfig.at(i*3+6) / 2.0f / 128.0f; // Pitch in halftones
				// Entry 4 contains the delay variation in 5ms steps the entropy pool is used to do the variation
				float delay, amplitude;
				if (i == 0) {
					delay = 0.0; 
					amplitude = 1.0f + (float)effectConfig.at(5) / 100.0f;
				}
				else if (i == 1) {
					delay = (float)effectConfig.at(4) / 200.0f; 
					amplitude = 1.0f - (float)effectConfig.at(5) / 100.0f;
				}
				else {
					delay = (float)((int)effectConfig.at(4) * (128 - (int)effectConfig.at(i*3+7)) * 5) / 1000.0f / 256.0f;
					// Entry 5 contains the amplitude variation in %
					amplitude = 1.0f + (float)effectConfig.at(5) * (float)effectConfig.at(i*3+7) / 100.0f / 128.0f; // USe the same entropy for delay and for amplitude
				}
				//qDebug("SoundFx: Voice Nr. %d, pitch: %f, delay: %f, amplitude: %f", i, pitch, delay, amplitude);
				getFromInputBuffer(frame1, WINDOW_COSINE, delay, powf(2.0f, pitch/12.0f), amplitude / sqrt(voices));
				patchToOutputBuffer(frame1);
			}
			offset += FRAMESIZE;
		break;
		default:
			qWarning("SoundFx: Unknown voice effect requested!");
		break;
	}
	// Get the data out of the buffer and return it (also same for all effects)
	getFromOutputBuffer(data);
}

Mono SoundFx::getDelayed(int stream, int index, float delay)
{
	assert(stream >=0 && stream < MAXSTREAMS);

	if (delay > DELAYLINELENGTHMS / 1000.0) delay = DELAYLINELENGTHMS / 1000.0;
	if (delay < 0.0 ) delay = 0.0; // Cannot mess with causality
	int lowerindex = ( (int)(-delay * (float) SAMPLERATE) + delaylinepos - 1 - FRAMESIZE + index + DELAYLINELENGTH) % DELAYLINELENGTH;
	int upperindex = ( (int)(-delay * (float) SAMPLERATE) + delaylinepos     - FRAMESIZE + index + DELAYLINELENGTH) % DELAYLINELENGTH;
	assert (upperindex < DELAYLINELENGTH);
	assert (upperindex >= 0);
	assert (lowerindex < DELAYLINELENGTH);
	assert (lowerindex >= 0);
	float fade = delay * (float) SAMPLERATE - floorf(delay * (float) SAMPLERATE);
	return delayline[stream][lowerindex] * (1.0 - fade) + delayline[stream][upperindex] * fade;
}

void SoundFx::initDirectivity(float angle[MAXSTREAMS])
{
	for (int p=0; p<MAXSTREAMS; p++) {
		delay_l[p] =  sin(angle[p] / 180.0 * M_PI) * HEADDELAY/2.0 + HEADDELAY/2.0;
		delay_r[p] = -sin(angle[p] / 180.0 * M_PI) * HEADDELAY/2.0 + HEADDELAY/2.0;
		lowpass_l[p] = angle[p] / 180.0;
		if (lowpass_l[p] < 0.0) lowpass_l[p] = 0.0;
		lowpass_r[p] = -angle[p] / 180.0;
		if (lowpass_r[p] < 0.0) lowpass_r[p] = 0.0;
	}
}

Stereo SoundFx::getWithDirectivity(int stream, int index)
{
	assert(index >=0 && index < DELAYLINELENGTH);
	assert(stream >=0 && stream < MAXSTREAMS);
	Stereo s;
	
	s.l = getDelayed(stream, index, delay_l[stream]);
	s.r = getDelayed(stream, index, delay_r[stream]);
	// The wider the angle the more high frequency portion is absorbed.
	// Divide by two since each ear gets input from two channels
	s.l = (lp_l[stream]->filt(s.l) * lowpass_l[stream] + s.l * (1.0 - lowpass_l[stream])) / 2.0;
	s.r = (lp_r[stream]->filt(s.r) * lowpass_r[stream] + s.r * (1.0 - lowpass_r[stream])) / 2.0;
	return s;
}

void SoundFx::applyComEffect(Mono * in, Mono *out, ComEffect effect, int size)
{
	switch (effect) {
		case COMEFFECT_NONE: qFatal("SoundFx: Requested to apply no comm effect!"); break;// Pass through - we should not be bothered to process this
		case COMEFFECT_MUTE: memset((void*)out, 0, size * sizeof(Mono)); break;// Mute
		case COMEFFECT_FAR: memcpy((void*)out, (void*)in, size*sizeof(Mono)); qDebug("applyComEffect: Implement me!"); break;
		case COMEFFECT_RADIO: 
			memcpy((void*)out, (void*)in, size*sizeof(Mono)); 
			for (int i=0; i<size; i++) out[i] += ((float) rand() / (float) RAND_MAX -0.5f)  / 10; // Add some noise
			break;
		case COMEFFECT_PHONE: memcpy((void*)out, (void*)in, size*sizeof(Mono)); break; // Just copy, the audio bandwidth limiting is done with the opus-codec
	}
}

QStringList SoundFx::getComEffectNames()
{
	QStringList names;
	names << "Select Com Effect" << "Far Away" << "Radio Link" << "Phone";
	return names;
}

LowPass::LowPass()
{
	m = 0.0;
}

void LowPass::setCornerFrequency(float freq)
{
	assert (freq >= 0);
	assert(freq < 24000.0);
	if (freq == 0) tau = -1;
	else tau = float(SAMPLERATE) / (2.0 * M_PI * freq);
}

Mono LowPass::filt(Mono s)
{
	if (tau < 0) m = s;
	else m += (s - m) / tau; 
	return m;
}

