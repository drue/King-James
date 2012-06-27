/*
 *  Transport.cc
 *  
 *
 *  Created on Mon Oct 01 2001.
 *  Copyright (c) 2001-2012 Andrew Loewenstern. All rights reserved.
 *
 */

#include "Transport.h"
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <algorithm>
#include <sys/prctl.h>

#include <fcntl.h>
#include <FLAC/all.h>

#include <jack/ringbuffer.h>

#include "const.h"

#define PREROLL_LENGTH 60
#define SAMPLE_SIZE 4 // used plughw to get signed 32-bit ints, since that's what FLAC wants


#ifndef timermsub
#define	timermsub(a, b, result) \
do { \
	(result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
	(result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec; \
	if ((result)->tv_nsec < 0) { \
		--(result)->tv_sec; \
		(result)->tv_nsec += 1000000000L; \
	} \
} while (0)
#endif

void AlsaTPort::suspend(void)
{
	int res;

    fprintf(stderr, "Suspended. Trying resume. "); fflush(stderr);
	while ((res = snd_pcm_resume(handle)) == -EAGAIN)
		sleep(1);	/* wait until suspend flag is released */
	if (res < 0) {
      fprintf(stderr, "Failed. Restarting stream. "); fflush(stderr);
      if ((res = snd_pcm_prepare(handle)) < 0) {
        fprintf(stderr, "suspend: prepare error: %s", snd_strerror(res));
        exit(-1);
      }
    }
    fprintf(stderr, "Done.\n");
}

void AlsaTPort::xrun(void)
{
	snd_pcm_status_t *status;
	int res;
	
	snd_pcm_status_alloca(&status);
	if ((res = snd_pcm_status(handle, status))<0) {
      printf("status error: %s", snd_strerror(res));
      exit(-1);
	}
	if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
        struct timespec now, diff, tstamp;
        clock_gettime(CLOCK_MONOTONIC, &now);
        snd_pcm_status_get_trigger_htstamp(status, &tstamp);
        timermsub(&now, &tstamp, &diff);
        fprintf(stderr, "%s!!! (at least %.3f ms long)\n",
				"overrun",
				diff.tv_sec * 1000 + diff.tv_nsec / 10000000.0);
        fprintf(stderr, "Status:\n");
        snd_pcm_status_dump(status, log);
        if ((res = snd_pcm_prepare(handle))<0) {
          printf("xrun: prepare error: %s", snd_strerror(res));
          exit(-1);
        }
        if ((res = snd_pcm_start(handle))<0) {
          printf("xrun: start error: %s", snd_strerror(res));
        }
        return;		/* ok, data should be accepted again */
	} if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {
      fprintf(stderr, "Status(DRAINING):\n");
      snd_pcm_status_dump(status, log);
      fprintf(stderr, "capture stream format change? attempting recover...\n");
      if ((res = snd_pcm_prepare(handle))<0) {
        fprintf(stderr, "xrun(DRAINING): prepare error: %s", snd_strerror(res));
        exit(-1);
      }
      return;
	}
    fprintf(stderr, "Status(R/W):\n");
    snd_pcm_status_dump(status, log);
    printf("read/write error, state = %s", snd_pcm_state_name(snd_pcm_status_get_state(status)));
	exit(-1);
}

void AlsaTPort::tick(snd_pcm_sframes_t (*reader)(snd_pcm_t *handle, void *buf, snd_pcm_uframes_t frames)) {
  snd_pcm_sframes_t count, r, got;
  snd_pcm_uframes_t n;
  jack_ringbuffer_data_t writevec[2];

  got = 0;
  count =  (sample_rate  / update_interval) * sizeof(FLAC__int32) * channels / 8;
  while(count > 0) {
    jack_ringbuffer_get_write_vector(ring, writevec);
    n = std::min((int)writevec[0].len / SAMPLE_SIZE / channels, (int)count);
    r = reader(handle, writevec[0].buf, n);
    if (r == -EPIPE) {
      xrun();
    } 
    else if (r == -ESTRPIPE) {
      suspend();
    }
    else if (r < 0) {
      fprintf(stderr, "read error: %s", snd_strerror(r));
      exit(-1);
    }
    else {
      got += r;
      count -= r;

      if (count && ((r * SAMPLE_SIZE * channels) == writevec[0].len) && writevec[1].len > 0) {
        r = reader(handle, writevec[1].buf, count);
        if (r == -EPIPE) {
          xrun();
        } else if (r == -ESTRPIPE) {
          suspend();
        } else if (r < 0) {
          fprintf(stderr, "read error: %s", snd_strerror(r));
          exit(-1);
        }
        else {
          got += r;
          count -=r;
        }
      }
      jack_ringbuffer_write_advance(ring, got * SAMPLE_SIZE * channels);
      pthread_cond_signal (&(data_ready));
      pthread_mutex_unlock (&(meter_lock));
      got = 0;
    }
  }
}

void* AlsaTPort::process(void *user)
{
  AlsaTPort *tport = (AlsaTPort *) user;
  struct sched_param schp;

  prctl(PR_SET_NAME, "card drain", 0, 0, 0);

  memset(&schp, 0, sizeof(schp));
  schp.sched_priority = sched_get_priority_max(SCHED_FIFO);

  if (sched_setscheduler(0, SCHED_FIFO, &schp) != 0) {
    perror("sched_setscheduler");
  }

  while(tport->stop_flag == false) {
   tport->tick(snd_pcm_readi);
  }
  return NULL;
}

AlsaTPort::AlsaTPort(const char *card, unsigned int bits_per_sample, unsigned int sample_rate, unsigned int update,  unsigned int ringlen=0, bool run=true) {
  char cardString[256];
  int err;
  snd_pcm_status_t *status;

  this->bits_per_sample = bits_per_sample;
  this->sample_rate = sample_rate;
  this->channels = 2;
  this->process_flag = 0;
  this->update_interval = update;
  this->spawns = run;

  if (ringlen == 0) {
    this->ring_length = SAMPLE_SIZE * channels * sample_rate; // one second
  }
  else {
    ring_length = ringlen;
  }
  sprintf(cardString, "plughw:%s", card);

  // set to 1 if threads should stop
  stop_flag = false;

  pthread_mutex_init(&meter_lock, NULL);
  pthread_cond_init(&data_ready, NULL);

  overruns = 0;

  ring = jack_ringbuffer_create(ring_length);
  jack_ringbuffer_mlock(ring);
  memset(ring->buf, 0, ring->size); // need to touch memory to ensure all pages are locked


  prerollSize = PREROLL_LENGTH * update_interval;
#ifdef DEBUG
  printf("prerollbuffer: %d\n", prerollSize);
#endif

  
  spool = new Spool(prerollSize, (sample_rate  / update_interval) * sizeof(FLAC__int32) * channels, this->bits_per_sample, this->sample_rate, channels, run);
  meter = new Meter(channels, sample_rate, ring, spool, &meter_lock, &data_ready);

  if (run) {
      err=snd_pcm_open(&handle, cardString, SND_PCM_STREAM_CAPTURE, 0);
      if (err < 0) {
        printf("audio open error: %s\n", snd_strerror(err));
      }
      
      setup();

      meter->start();
      pthread_create(&cthread, NULL, process, this);
      
      err = snd_pcm_start(handle);
      if(err != 0) {
        printf("did not start.\n");
      }

      snd_pcm_status_alloca(&status);
      if ((err = snd_pcm_status(handle, status))<0) {
        printf("status error: %s", snd_strerror(err));
        exit(-1);
      }

      printf("state: %d\n", snd_pcm_status_get_state(status));
#ifdef DEBUG
      printf("ready...\n");
#endif
    }
}

void AlsaTPort::setup() {
  snd_pcm_hw_params_t *params;
  snd_pcm_sw_params_t *swparams;
  snd_pcm_uframes_t buffer_size;
  int err;
  snd_pcm_uframes_t chunk_size;
  unsigned int rate = 48000;
  unsigned int buffer_time = 0;
  unsigned int period_time = 0;

  snd_pcm_hw_params_alloca(&params);
  snd_pcm_sw_params_alloca(&swparams);

  snd_output_stdio_attach(&log, stderr, 0);

  err = snd_pcm_hw_params_any(handle, params);
  if (err < 0) {
    printf("Bad configuration for this PCM: no configurations available\n");
    exit(-1);
  }

  snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S24);
    
  snd_pcm_hw_params_set_channels(handle, params, channels);


  snd_pcm_hw_params_set_rate_near(handle, params, &rate, 0);

  snd_pcm_hw_params_get_buffer_time_max(params, &buffer_time, 0);
    
  assert(err >= 0);
  if (buffer_time > 500000)
    buffer_time = 500000;

  period_time = (sample_rate  / update_interval / 4) * sizeof(FLAC__int32) * channels;

  err=snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, 0);
  assert(err >= 0);

  buffer_time = period_time * 8;

  err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, 0);
  assert(err >= 0);
  
  err = snd_pcm_hw_params(handle, params);

  if (err < 0) {
    printf("Unable to install hw params:\n");
    snd_pcm_hw_params_dump(params, log);
    exit(-1);
  }

  printf("snd_pcm_hw_params_dump:\n");
  snd_pcm_hw_params_dump(params, log);

  snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
  snd_pcm_hw_params_get_buffer_size(params, &buffer_size);

  snd_pcm_sw_params_current(handle, swparams);

  snd_pcm_sw_params_set_avail_min(handle, swparams, chunk_size);
  snd_pcm_sw_params_set_start_threshold(handle, swparams, 0);
  
  //  err = snd_pcm_sw_params_set_xfer_align(handle, swparams, 4);

  //  snd_pcm_sw_params_set_stop_threshold(handle, swparams, buffer_size);

  if (snd_pcm_sw_params(handle, swparams) < 0) {
    printf("unable to install sw params:\n");
    snd_pcm_sw_params_dump(swparams, log);
    exit(-1);
  }

  printf("snd_pcm_dump:\n");
  snd_pcm_dump(handle, log);
}

AlsaTPort::~AlsaTPort() {
  delete(meter);
  delete(spool);
  jack_ringbuffer_free(ring);
}

void AlsaTPort::startRecording(char *path) {
  spool->start(path);
}

void AlsaTPort::stopRecording() {
  Spool *oldSpool = spool;
  spool = new Spool(prerollSize, (sample_rate  / update_interval) * sizeof(FLAC__int32) * channels, this->bits_per_sample, this->sample_rate, channels, spawns);
  meter->switchSpool(spool);
  oldSpool->finish();
}

void AlsaTPort::wait()
{
  int n = 0;
  while(n < 1) {
    sem_wait(&finished_sem);
    n++;
  }
}

void AlsaTPort::stop()
{

}

int AlsaTPort::gotSignal() {
  return false;
}

void AlsaTPort::doReturn()
{
  stop_flag = true;
}

long AlsaTPort::getmaxn(unsigned int n) 
{
  return meter->getmaxn(n);
}

void  AlsaTPort::resetmax() 
{
  meter->resetmax();
}
