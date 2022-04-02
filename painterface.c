#include <pulse/pulseaudio.h>
//#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "painterface.h"

#define MONO 1
#define STEREO 2l
#define BYTESPERSAMPLE 2l

#define MAXBUFFERSIZE 65536l
#define TIMEOUT 100

int buffers = 7;
int thresh = 1500;
int latency = 0;

// From pulsecore/macro.h
#define pa_memzero(x,l) (memset((x), 0, (l)))
#define pa_zero(x) (pa_memzero(&(x), sizeof(x)))

float * inframe, * outframe; // n-time buffered input and output data
int fs;

bool verbose = false;
bool testsoundout = false;
bool testsoundin = false;


static void *ibuffer = NULL;
volatile int ibufferpos = 0;
volatile int iblocktoread = 0;

static void *obuffer = NULL;
volatile int obufferpos = 0;
volatile int oblocktowrite;

static void *buffer = NULL;
static size_t buffer_length = 0, buffer_index = 0;

static pa_sample_spec sample_spec;


static pa_stream *istream = NULL;
static pa_stream *ostream = NULL;

static uint32_t flags = 0;

// Define our pulse audio loop and connection variables
//pa_threaded_mainloop *pa_ml;
static pa_mainloop *pa_ml;
static pa_mainloop_api *pa_mlapi;
static pa_context *pa_ctx;

// This is my builtin card. Use paman to find yours
//static char *idevice = "alsa_input.pci-0000_00_1b.0.analog-stereo";
// static char *idevice = "alsa_input.pci-0000_00_14.2.analog-stereo";
char *idevice = NULL; // Use whatever is available
//static char *idevice ="alsa_input.usb-0c76_USB_PnP_Audio_Device_EEPROM_-00-DeviceEEPROM.analog-mono";
//static char *odevice = "alsa_output.pci-0000_00_1b.0.analog-stereo";
//static char *odevice = "alsa_output.pci-0000_00_14.2.analog-stereo";
char *odevice = NULL; // Use whatever is available
//static char *odevice = "alsa_output.usb-0d8c_C-Media_USB_Headphone_Set-00-Set.analog-stereo";



static void stream_state_callback(pa_stream *s, void *userdata) 
{
	(void)userdata;
	assert(s);
	if (verbose) fprintf(stderr, "Stream State Callback\n");
	switch (pa_stream_get_state(s)) {
	case PA_STREAM_CREATING:
		if (verbose) fprintf(stderr, "Creating stream\n");
		buffer = pa_xmalloc(MAXBUFFERSIZE * BYTESPERSAMPLE * STEREO);
		buffer_length = MAXBUFFERSIZE * BYTESPERSAMPLE * STEREO;
		buffer_index = 0;
		break;
	case PA_STREAM_TERMINATED:
		if (verbose) fprintf(stderr, "Stream terminated\n");
		break;
	case PA_STREAM_READY:
    // Just for info: no functionality in this branch
    if (verbose) {
		const pa_buffer_attr *a;
		//char cmt[PA_CHANNEL_MAP_SNPRINT_MAX], sst[PA_SAMPLE_SPEC_SNPRINT_MAX];
		fprintf(stderr, "Stream successfully created.\n");
		if (!(a = pa_stream_get_buffer_attr(s))) fprintf(stderr,"pa_stream_get_buffer_attr() failed: %s", pa_strerror(pa_context_errno(pa_stream_get_context(s))));
		else printf("Buffer metrics: maxlength=%u, fragsize=%u\n", a->maxlength, a->fragsize);
		fprintf(stderr, "Connected to device %s (%u, %ssuspended)\n", pa_stream_get_device_name(s), pa_stream_get_device_index(s), pa_stream_is_suspended(s) ? "" : "not ");
    }
    break;
	case PA_STREAM_FAILED:
	default:
		fprintf(stderr, "Stream error: %s", pa_strerror(pa_context_errno(pa_stream_get_context(s))));
		exit(1); // FIXME: proper error handling!
	}
}

// This is called whenever new data is available 
static void stream_read_callback(pa_stream *s, size_t length, void *userdata) 
{
	static float phase = 0;
	(void) userdata;
	const int16_t *idata;
	if (verbose) fprintf(stderr, "painterface: Read callback! Readable: %d\n", (int)length);
    if (length == 0) return;
	if (pa_stream_peek(s, (const void **)&idata, &length) < 0) {
		fprintf(stderr, "painterface: Read failed\n");
		return;
    }
	pa_stream_drop(s);
	if (idata == NULL) {
		// We've got a hole!
		if (true) fprintf(stderr, "painterface: Hole detected!\n");
		return;
	}
    if (verbose) fprintf(stderr, "painterface: Processing %ld input samples\n", (int)length / BYTESPERSAMPLE / STEREO);

    // Transfer the processed data to the output buffer and the recorded
    // data from the input buffer as new data to process
	if (testsoundin) {
		for (unsigned int i = 0; i < length / BYTESPERSAMPLE / STEREO; i++) {
			inframe[ibufferpos] = sin(phase) * 0.1; // Will always be mono - only one mouth!
			ibufferpos = (ibufferpos + 1) % (fs * buffers); // Simpy read. Ignore buffer overrun! When the app cannot keap trac it does not deserve proper realtie data!
			phase += 440.0/48000*2.0*M_PI;
			if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
		}
	} else {
		for (unsigned int i = 0; i < length / BYTESPERSAMPLE / STEREO; i++) {
			inframe[ibufferpos] = ((float)(idata[i * STEREO])+ (float)(idata[i * STEREO + 1]))/65536.0f; // Will always be mono - only one mouth!
			ibufferpos = (ibufferpos +1) % (fs * buffers); // Simpy read. Ignore buffer overrun! When the app cannot keap trac it does not deserve proper realtie data!
		}
	}
	if (verbose) fprintf(stderr, "painterface: Bufferpos: %d\n", ibufferpos);
}

/* This is called whenever new data may be written to the stream */
static void stream_write_callback(pa_stream *s, size_t length, void *userdata) 
{
	static float phase = 0;
	int obufferlevel;
	static int obufferlevelcontrol = 0;
	unsigned int i;
	static int16_t odata[MAXBUFFERSIZE * STEREO * BYTESPERSAMPLE]; // won't need more  
	(void)userdata; // Don't want to get warnings 
	if (verbose) fprintf(stderr, "painterface: Write callback. Writable: %lu\n", length);
	assert (length <= MAXBUFFERSIZE * STEREO * BYTESPERSAMPLE); // Just to be sure!
	assert (s != NULL);
	if (length == 0) return; // Just in case we got a callback but nothing to write
	if (testsoundout) {
		for (i=0; i < length / STEREO / BYTESPERSAMPLE; i++){
			odata[i * STEREO]     = (int16_t)(sin(phase) * 5000.0);
			odata[i * STEREO + 1] = (int16_t)(sin(phase) * 5000.0);
			phase += 440.0/48000*2.0*M_PI;
			if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
		}
	} else {
		obufferlevel = (obufferpos - oblocktowrite * fs + fs * buffers) % (fs * buffers);
		//if (verbose) fprintf(stderr, "painterface:                                  Outputbufferlevel: %d\n", obufferlevel - (buffers/2) * fs);
		// Try to keep the output buffer level around the center of the total buffer
		obufferlevelcontrol += obufferlevel - (buffers/2) * fs; 
		//fprintf(stderr, "painterface:                                  Outputbufferlevelcontrol: %d\n", obufferlevelcontrol);
		for (i=0; i < length / STEREO / BYTESPERSAMPLE; i++){
			odata[i * STEREO]     = (int16_t)(outframe[obufferpos * STEREO] * 32768.0f);
			odata[i * STEREO + 1] = (int16_t)(outframe[obufferpos * STEREO + 1] * 32768.0f);
			if (i == 0 && obufferlevelcontrol < -thresh) {
				// Buffer is always a little too full
				obufferlevelcontrol = 0;
				// skip one sample from the buffer
				obufferpos = (obufferpos + 2) % (fs * buffers);
				if (verbose) fprintf(stderr, "painterface: Skipping one sample for in/out sync\n");
			}
			else if (i == 0 && obufferlevelcontrol > thresh) {
				// Buffer is always a little too empty
				if (verbose) fprintf(stderr, "painterface: Doubling one sample for in/out sync\n");
				obufferlevelcontrol = 0;
				// send one sample two times
				//obufferpos = (obufferpos + 0) % (fs * buffers);
			}
			else {
				obufferpos = (obufferpos + 1) % (fs * buffers); // Normal buffer increment
			}
		}
	}
	if (pa_stream_write(ostream, (uint8_t*) odata, length, NULL, 0, PA_SEEK_RELATIVE) < 0) {
		fprintf(stderr, "pa_stream_write() failed\n");
		exit(-4);
	}
	return;
}

// This callback gets called when our context changes state.  We really only
// care about when it's ready or if it has failed
static void state_cb(pa_context *c, void *userdata) 
{
	pa_context_state_t state;
	int *pa_ready = (int *)userdata;

	if (verbose) fprintf(stderr, "State changed\n");
	state = pa_context_get_state(c);
	switch  (state) {
		// There are just here for reference
		case PA_CONTEXT_UNCONNECTED:
			if (verbose) printf("UNCONNECTED\n");
		break;
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
		default:
			break;
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			*pa_ready = 2;
		break;
		case PA_CONTEXT_READY: {
			pa_buffer_attr buffer_attr;
			if (verbose) fprintf(stderr,"Connection established.\n");
			if (!(ostream = pa_stream_new(c, "gotongiPlayback", &sample_spec, NULL))) {
				fprintf(stderr, "pa_stream_new() failed: %s", pa_strerror(pa_context_errno(c)));
				exit(1);
			}
			if (!(istream = pa_stream_new(c, "gotongiCapture", &sample_spec, NULL))) {
				fprintf(stderr, "pa_stream_new() failed: %s", pa_strerror(pa_context_errno(c)));
				exit(1);
			}
			// Watch for changes in the stream state to create the output file
			pa_stream_set_state_callback(istream, stream_state_callback, NULL);
			// Watch for changes in the stream's read state to write to the output file
			pa_stream_set_read_callback(istream, stream_read_callback, NULL);

			pa_stream_set_write_callback(ostream, stream_write_callback, NULL);
	

			// Set properties of the record buffer
			pa_zero(buffer_attr);
			buffer_attr.maxlength = (uint32_t) fs * STEREO * BYTESPERSAMPLE * (2+latency * 2); // Don't ask me - works best for me!
			buffer_attr.prebuf = (uint32_t) 128 + latency * 128;
			buffer_attr.fragsize = 64;
			buffer_attr.tlength = 64;
			//flags |= PA_STREAM_ADJUST_LATENCY;
			flags |= PA_STREAM_EARLY_REQUESTS;
			buffer_attr.minreq = 32;

			// and start recording
			if (pa_stream_connect_record(istream, idevice, &buffer_attr, (pa_stream_flags_t)flags) < 0) {
				fprintf(stderr, "pa_stream_connect_record() failed: %s", pa_strerror(pa_context_errno(c)));
				exit(1);
			}
			
			// start playback
			if (pa_stream_connect_playback(ostream, odevice, &buffer_attr, (pa_stream_flags_t)flags, NULL, NULL) < 0) {
				fprintf(stderr, "pa_stream_connect_playback() failed: %s", pa_strerror(pa_context_errno(c)));
				exit(1); //goto fail;
			} 
			if (verbose) fprintf(stderr, "Set playback callback\n");
			if (verbose) fprintf(stderr, "Set record callback\n");
		}
		break;
	}
}

bool pa_init(char * id, char * od, uint32_t framesize, uint32_t samplerate, int latencylevel)
{
    int err;
	if (verbose) fprintf(stderr, "pa_init called!\n");
	assert(framesize <= 65536);
	fs = framesize;
	assert (latencylevel>=0);
	assert (latencylevel<=4);
	latency = latencylevel;
	switch (latency) {
		case 0: buffers = 3; thresh = 1000; break;
		case 1: buffers = 5; thresh = 1500; break;
		case 2: buffers = 7; thresh = 2000; break;
		case 3: buffers = 9; thresh = 2500; break;
		case 4: buffers = 11; thresh = 5000; break;
	}
	
	oblocktowrite = buffers / 2;
	
	ibuffer = (int16_t*) pa_xmalloc(MAXBUFFERSIZE * BYTESPERSAMPLE * MONO);
	obuffer = (int16_t*) pa_xmalloc(MAXBUFFERSIZE * BYTESPERSAMPLE * STEREO);

	ibufferpos = 0;
	obufferpos = 0;
	iblocktoread = 0;
	oblocktowrite = 2;
	
	inframe = (float*)pa_xmalloc(fs * buffers * MONO * sizeof(float));
	assert(inframe != NULL);
	memset((void *)inframe, 0, fs * buffers * MONO * sizeof(float));
	
	outframe = (float*)pa_xmalloc(fs * buffers * STEREO * sizeof(float));
	assert(outframe != NULL);
	memset((void *)outframe, 0, fs * buffers * STEREO * sizeof(float));

	idevice = id;
	odevice = od;
	sample_spec.format = PA_SAMPLE_S16LE;
	sample_spec.rate = samplerate;
	sample_spec.channels = STEREO;
    
	// Create a mainloop API and connection to the default server
	if (verbose) fprintf(stderr, "pa_init: Creating mainloop\n");
	//pa_ml = pa_threaded_mainloop_new();
	pa_ml = pa_mainloop_new();
	if (verbose) fprintf(stderr, "pa_init: Getting API\n");
	//pa_mlapi = pa_threaded_mainloop_get_api(pa_ml);
	pa_mlapi = pa_mainloop_get_api(pa_ml);
	if (verbose) fprintf(stderr, "pa_init: Creating new context\n");
    pa_ctx = pa_context_new(pa_mlapi, "gotongi");
	
	//pa_threaded_mainloop_set_name(pa_ml,"gotongiPAThread");
	// This function connects to the pulse server
	if (verbose) fprintf(stderr, "pa_init: Connecting context\n");
	err = pa_context_connect(pa_ctx, NULL, (pa_context_flags_t)0, NULL);
    if (err < 0) fprintf(stderr, "Error: %d\n", err);
	// This function defines a callback so the server will tell us its state.
	if (verbose) fprintf(stderr, "pa_init: Activating state callback\n");
    pa_context_set_state_callback(pa_ctx, state_cb, NULL);
    
    //if (verbose) fprintf(stderr, "pa_init: Starting threaded mainloop\n");
	//pa_threaded_mainloop_start(pa_ml);

	if (verbose) fprintf(stderr, "pa_init done!\n");
	return true;
}

void pa_exit(void)
{
	//pa_threaded_mainloop_stop(pa_ml);
	pa_context_disconnect(pa_ctx);
	//usleep(10000);
	free(inframe);
	free(outframe);
}

void pa_readframe(float *in)
{
    int ret;
	// The block named active block is the one that's actually been processed by the application
	// If the request is too early wait till one buffer element is full
	if (verbose) fprintf(stderr, "pa_read called! Active block to read: %d\r\n", iblocktoread);
	do {
		pa_mainloop_iterate(pa_ml, 0, &ret); // Call the mainloop till we've got enough data in our buffer - we must wait anyway
		if (latency > 0) usleep(0);
	}
	while (ibufferpos / fs == iblocktoread);
	
	// Make a deep copy to ensure no messing with the data during processing
	memcpy((void *)in, (void *)&inframe[iblocktoread * fs], fs * sizeof(float)); 
	iblocktoread = (iblocktoread+1) % buffers; // FIXME: Do I have to do this thread safe?
}

void pa_writeframe(float *out)
{
    int ret;
	if (verbose) fprintf(stderr, "pa_write called! Active block to write: %d\r\n", oblocktowrite);
	memcpy((void *)&outframe[oblocktowrite * fs * STEREO], (void *) out, fs * sizeof(float) * STEREO);
	oblocktowrite = (oblocktowrite+1) % buffers; // FIXME: Do I have to do this thread safe?
	pa_mainloop_iterate(pa_ml, 0, &ret); // Just a try. Does it work better, when we allow the main loop to do its work here too?
}

