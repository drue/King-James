/*
 *  port.cc
 *  
 *
 *  Created on Mon Oct 01 2001.
 *  Copyright (c) 2002 Andrew Loewenstern. All rights reserved.
 *
 */

#include "port.h"
#include <sys/time.h>
#include <pthread.h>

#define PERIOD 256
#define PERIODS 20
#define BUFFER PERIOD * PERIODS

APort::APort(){};

APort::APort(int direction, unsigned int card, unsigned int bits_per_sample, unsigned int sample_rate)
{
  snd_pcm_hw_params_t *parms;
  snd_pcm_sw_params_t *swparams;
  snd_pcm_info_t *info;
  int err, x;
  char id[64];
  char cardString[256];

  sprintf(cardString, "plughw:%u,0", card);

  this->bits_per_sample = bits_per_sample;
  this->sample_rate = sample_rate;

  if(direction == CAPTURE){
    err=snd_pcm_open(&(handle), cardString, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
    strcpy(id, "CAPTURE");
  }
  else {
    err=snd_pcm_open(&(handle), cardString, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
    strcpy(id, "PLAYBACK");
  }
    
  if (err < 0) { 
    printf("snd_pcm_open error: %s", snd_strerror(err)); 
    exit(-1);
  } 
    
  snd_pcm_info_alloca(&info);
  if ((err = snd_pcm_info(handle, info)) < 0) { 
    printf("snd_pcm_info error: %s", snd_strerror(err)); 
    exit(-1);
  } 

  snd_pcm_hw_params_alloca(&parms);
  err = snd_pcm_hw_params_any(handle, parms);
    
   
  format = SND_PCM_FORMAT_S24;
  err = snd_pcm_hw_params_set_access(handle, parms,
                                     SND_PCM_ACCESS_RW_INTERLEAVED);
    
  err = snd_pcm_hw_params_set_format(handle, parms, format);
  err = snd_pcm_hw_params_set_channels(handle, parms, 2);
  x = 0;
  sample_rate = snd_pcm_hw_params_set_rate_near(handle, parms, &sample_rate, &x);
    
  /*    if ((err = snd_pcm_hw_params_set_period_size (handle, parms, 8, 0)) < 0) {
        printf("Can't set periods to 8.\n");
        }

    
        if (period_time < 0)
        period_time = buffer_time;    
        period_time = snd_pcm_hw_params_set_period_size(handle, parms,
        PERIOD, 0);
        #ifdef DEBUG
        assert(period_time >= 0);
        printf("period_time: %d\n", period_time);
        #endif

        buffer_time = snd_pcm_hw_params_set_buffer_size(handle, parms, BUFFER);
        #ifdef DEBUG
        assert(buffer_time >= 0);
        printf("buffer_time: %d\n", buffer_time);
        #endif
  */
    
  err = snd_pcm_hw_params(handle, parms);

  snd_pcm_hw_params_get_period_size(parms, &period_time, 0);
  bits_per_sample = snd_pcm_format_physical_width(format);
  bits_per_frame = bits_per_sample * 2;
  period_bytes =  period_time * (bits_per_frame) / 8;

#ifdef DEBUG
  printf("bps: %u pt: %lu period_bytes: %d\n", bits_per_sample, period_time, period_bytes);
#endif
    

  // swparms
  snd_pcm_sw_params_alloca(&swparams);
  err = snd_pcm_sw_params_current(handle, swparams);
  if (err < 0) {
    printf("Unable to determine current swparams for %s: %s\n", id, snd_strerror(err));
  }
  if (direction == CAPTURE)
    err = snd_pcm_sw_params_set_start_threshold(handle, swparams, 0);
  else
    err = snd_pcm_sw_params_set_start_threshold(handle, swparams, 0x7FFFFFFF);

  if (err < 0) {
    printf("Unable to set start threshold mode for %s: %s\n", id, snd_strerror(err));
  }
  err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_time);
  if (err < 0) {
    printf("Unable to set avail min for %s: %s\n", id, snd_strerror(err));
  }
  err = snd_pcm_sw_params_set_xfer_align(handle, swparams, 4);
  if (err < 0) {
    printf("Unable to set transfer align for %s: %s\n", id, snd_strerror(err));
  }
  err = snd_pcm_sw_params(handle, swparams);
  if (err < 0) {
    printf("Unable to set sw params for %s: %s\n", id, snd_strerror(err));
  }
    
}

APort::~APort() {
  snd_pcm_drop(handle);
  snd_pcm_hw_free(handle);
  snd_pcm_close(handle);
}

int APort::getBufferSize() { return buffer_time; }
int APort::getPeriodBytes() { return period_bytes; }
int APort::getPeriodTime() { return period_time; }

void APort::prepare() {
  int err;
  err = snd_pcm_prepare(handle);
  if (err < 0) {
    printf("Unable to prepare %s: %s\n", "port", snd_strerror(err));
    dumpStatus();
  }

}


void APort::monitorOff(){
  snd_ctl_elem_value_t *val;
  snd_ctl_t *ctl;
  int err;

  if ((err = snd_ctl_open(&ctl, "hw:0", 0)) < 0) {
    printf("snd_ctl_open: %s\n", snd_strerror(err));
    return;
  }
#define SPDIF_PLAYBACK_ROUTE_NAME	"IEC958 Playback Route"
#define ANALOG_PLAYBACK_ROUTE_NAME	"H/W Playback Route"

  snd_ctl_elem_value_alloca(&val);
  snd_ctl_elem_value_set_interface(val, SND_CTL_ELEM_IFACE_MIXER);
  snd_ctl_elem_value_set_name(val, SPDIF_PLAYBACK_ROUTE_NAME);
  snd_ctl_elem_value_set_index(val, 0);
  snd_ctl_elem_value_set_enumerated(val, 0, 1);
  if ((err = snd_ctl_elem_write(ctl, val)) < 0)
    printf("Failed to set monitor route left spdif.\n");

  snd_ctl_elem_value_alloca(&val);
  snd_ctl_elem_value_set_interface(val, SND_CTL_ELEM_IFACE_MIXER);
  snd_ctl_elem_value_set_name(val, SPDIF_PLAYBACK_ROUTE_NAME);
  snd_ctl_elem_value_set_index(val, 1);
  snd_ctl_elem_value_set_enumerated(val, 0, 2);
  if ((err = snd_ctl_elem_write(ctl, val)) < 0)
    printf("Failed to set monitor route right spdif.\n");
}

void APort::monitorOn(){
  snd_ctl_elem_value_t *val;
  snd_ctl_t *ctl;
  int err;

  if ((err = snd_ctl_open(&ctl, "hw:0", 0)) < 0) {
    printf("snd_ctl_open: %s\n", snd_strerror(err));
    return;
  }
#define SPDIF_PLAYBACK_ROUTE_NAME	"IEC958 Playback Route"
#define ANALOG_PLAYBACK_ROUTE_NAME	"H/W Playback Route"

  snd_ctl_elem_value_alloca(&val);
  snd_ctl_elem_value_set_interface(val, SND_CTL_ELEM_IFACE_MIXER);
  snd_ctl_elem_value_set_name(val, SPDIF_PLAYBACK_ROUTE_NAME);
  snd_ctl_elem_value_set_index(val, 0);
  snd_ctl_elem_value_set_enumerated(val, 0, 9);
  if ((err = snd_ctl_elem_write(ctl, val)) < 0)
    printf("Failed to set monitor route left spdif.\n");

  snd_ctl_elem_value_alloca(&val);
  snd_ctl_elem_value_set_interface(val, SND_CTL_ELEM_IFACE_MIXER);
  snd_ctl_elem_value_set_name(val, SPDIF_PLAYBACK_ROUTE_NAME);
  snd_ctl_elem_value_set_index(val, 1);
  snd_ctl_elem_value_set_enumerated(val, 0, 10);
  if ((err = snd_ctl_elem_write(ctl, val)) < 0)
    printf("Failed to set monitor route right spdif.\n");

}

void APort::setSilence(){
  u_char *buf[2];
  int ret;

  buf[0] = (u_char *)malloc(period_bytes);
  buf[1] = (u_char *)malloc(period_bytes);
  snd_pcm_format_set_silence(format, buf[0], period_time);
  snd_pcm_format_set_silence(format, buf[1], period_time);
  ret = writeFromBuf(buf, period_time);
  if(ret != period_time)
    printf("silence didn't match period_time!\n");
  free(buf[0]);
  free(buf[1]);
}

int APort::start() {
  int err, res;
  snd_pcm_status_t *status;

  err = snd_pcm_start(handle);
  if (err == -5) {
    // no signal
    return 0;
  }
  else if (err < 0) {
    printf("Unable to start port - %d: %s\n", err, snd_strerror(err));
  }

  snd_pcm_status_alloca(&status);
  if ((res = snd_pcm_status(handle, status))<0) {
    printf("status error: %s", snd_strerror(res));
    exit(-1);
  }
  if (snd_pcm_status_get_state(status) == SND_PCM_STATE_RUNNING) {
    got_signal = 1;
    return 1;
  }
  return 0;
}

void APort::stop() {
  snd_pcm_drop(handle);
}


int APort::readIntoBuf(FLAC__int32 *buf, ssize_t count)
{
  snd_pcm_status_t *status;
  ssize_t r=0;
  size_t result =0;
  int res =  0;
  int err =  0;
  int x = 0;

  snd_pcm_status_alloca(&status);
  while(count > 0) {
    if(snd_pcm_avail_update(handle) > 0) {
      r = snd_pcm_readi(handle, (void *)buf + (result * 8), count);
      if (r == -EAGAIN || (r >= 0 && r < count)) {
        snd_pcm_wait(handle, 1000);
      }
      else if (r == -EPIPE) {
        xrun();
      } 
      else if (r == -ESTRPIPE){
        while ((r = snd_pcm_resume(handle)) == -EAGAIN)
          printf("STRPIPE\n");
          usleep(1000);	/* wait until suspend flag is released */
        if (res < 0) {
          if ((res = snd_pcm_prepare(handle)) < 0) {
#ifdef DEBUG
            printf("suspend: prepare error: %s", snd_strerror(res));
#endif
            exit(-1);
          }
        }
      }
      else     if (r < 0) {
        printf("read error2: %ld %s\n", r, snd_strerror(r));
        snd_pcm_wait(handle, 1000);
      }
    }
    else {
      if ((res = snd_pcm_status(handle, status))<0) {
        printf("status error: %s", snd_strerror(res));
        exit(-1);
      }

      if (snd_pcm_status_get_state(status) != SND_PCM_STATE_RUNNING) {      
        //dumpStatus();
      }

      if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {
        // no signal
        //	printf ("No Signal!\n");
        got_signal = 0;
        //snd_pcm_drop(handle);
        return -1;
      }
      if (snd_pcm_status_get_state(status) == SND_PCM_STATE_SETUP) {
        prepare();
        res = start();
        if (res) {
          return 0;
        }
        else {
          return -1;
        }
      }
      if (snd_pcm_status_get_state(status) == SND_PCM_STATE_PREPARED) {
        // no signal
        got_signal = 0;
        return -1;
      }
    }
    if (r > 0) {
      result += r;
      count -= r;
      x++;
    }
  }
  return result;
}

int APort::writeInterleavedFromBuf(u_char *buf, ssize_t count)
{
  ssize_t r=0;
  r = snd_pcm_writei(handle, buf, count);
  if (r == -EPIPE) {
    xrun();
  } else if (r < 0) {
    printf("write error: %s", snd_strerror(r));
  }
  return r;
}

int APort::writeFromBuf(u_char **buf, ssize_t count)
{
  ssize_t r=0;
  size_t result =0;
  int res =  0;
  int x = 0;

  //  while (count > 0) {
  void *tmp[2];
  tmp[0] = buf[0] + 3 * result;
  tmp[1] = buf[1] + 3 * result;

  r = snd_pcm_writen(handle, tmp, count);
  if (r == -EAGAIN || (r >= 0 && r < count)) {
    snd_pcm_wait(handle, 10000);
  } else if (r == -EPIPE) {
    xrun();
  } else if (r == -ESTRPIPE){
    while ((r = snd_pcm_resume(handle)) == -EAGAIN)
      usleep(500);	/* wait until suspend flag is released */
    if (res < 0) {
      if ((res = snd_pcm_prepare(handle)) < 0) {
#ifdef DEBUG
        printf("suspend: prepare error: %s", snd_strerror(res));
#endif
        exit(-1);
      }
    }
  }
  else if (r < 0) {
    printf("read error3: %s\n", snd_strerror(r));
    exit(-1);
  }
  if (r > 0) {
    result += r;
    count -= r;
    x++;
  }
  //  }
  return result;
}

void APort::xrun(void)
{
  snd_pcm_status_t *status;
  int res;
	
  snd_pcm_status_alloca(&status);
  if ((res = snd_pcm_status(handle, status))<0) {
    printf("xrun status error: %s", snd_strerror(res));
    exit(-1);
  }
  if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
    struct timeval now, diff, tstamp;
    gettimeofday(&now, 0);
    snd_pcm_status_get_trigger_tstamp(status, &tstamp);
    timersub(&now, &tstamp, &diff);
    time_t foo = time(NULL);
    printf("xrun!!! (at least %.3f ms long %s)\n",  diff.tv_sec * 1000 + diff.tv_usec / 1000.0, ctime(&foo));
    if ((res = snd_pcm_prepare(handle))<0) {
      printf("xrun: prepare error: %s", snd_strerror(res));
    }
    if ((res = snd_pcm_start(handle))<0) {
      printf("xrun: start error: %s", snd_strerror(res));
    }
    return;		/* ok, data should be accepted again */
  }
}

void APort::dumpStatus()
{
  snd_pcm_status_t *status;
  snd_output_t *out;
  int res;

  snd_pcm_status_alloca(&status);
  snd_output_stdio_attach(&out, stderr, 0);
  if ((res = snd_pcm_status(handle, status))<0) {
    printf("status error: %s", snd_strerror(res));
    exit(-1);
  }
  snd_pcm_status_dump(status, out);
}

int APort::linkWithPort(APort *port)
{
  return port->linkToHandle(this->handle);
}

int APort::linkToHandle(snd_pcm_t *h)
{
  return snd_pcm_link(h, this->handle);
}
