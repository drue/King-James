/*
 *  port.h
 *  
 *
 *  Created on Mon Oct 01 2001.
 *  Copyright (c) 2001 __CompanyName__. All rights reserved.
 *
 */

#include <alsa/asoundlib.h>
#include <FLAC/all.h>

#define CAPTURE 0
#define PLAYBACK 1

class APort {
    protected:
	snd_pcm_t *handle;
	snd_pcm_format_t format;
	int bits_per_sample, sample_rate;
	int buffer_time;
	snd_pcm_uframes_t period_time;
	int bits_per_frame;
	int period_bytes;
    public:
	int got_signal;
	APort(int direction, unsigned int card, unsigned int bits_per_sample, unsigned int sample_rate);
	APort();
	~APort();
	void prepare();
	int getBufferSize();
	int getBitsPerFrame() { return bits_per_frame;}
	int getPeriodBytes();
	int getPeriodTime();
    int readIntoBuf(FLAC__int32 *buf, ssize_t count);
	int writeInterleavedFromBuf(u_char *buf, ssize_t count);
	int writeFromBuf(u_char **buf, ssize_t count);
	int linkWithPort(APort *port);
	int linkToHandle(snd_pcm_t *h);
	void monitorOn();
	void monitorOff();
	void setSilence();
	void xrun();
	void dumpStatus();
	int start();
	void stop();
};
