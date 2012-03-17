/*
 *  Transport.cc
 *  
 *
 *  Created on Mon Oct 01 2001.
 *  Copyright (c) 2001-2010 Andrew Loewenstern. All rights reserved.
 *
 */

#include "Transport.h"
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <sys/types.h>
//#include <sys/stat.h>
#include <algorithm>

#include <fcntl.h>
#include <FLAC/all.h>

#include <jack/jack.h>
#include <jack/ringbuffer.h>

#include "const.h"

#define PREROLL_LENGTH 60
#define RING_LENGTH 5
#define UPDATE_INTERVAL 10 // hz

// this function is called by jack and should be at realtime priority
// it should do as little as possible as quickly as possible because nothing
// else happens while it is running
int AlsaTPort::process(jack_nframes_t nframes, void *user)
{
	int chn;
	size_t space = 0;
	AlsaTPort *tport = (AlsaTPort *) user;

    if(tport->process_flag == 0)
      return 0;

    // get buffers
	for (chn = 0; chn < tport->channels; chn++)
      tport->in[chn] = (jack_default_audio_sample_t*) jack_port_get_buffer (tport->ports[chn], nframes);
    
    // we have to write the same amount to every ring, otherwise the channels could get out of sync if
    // there is an overflow on only some channels because the drain is halfway through the rings
    // the ring should be large enough to always have enough space unless there is a problem
    space = jack_ringbuffer_write_space(tport->rings[0]);    
    for (int i=1;i<tport->channels;i++) {
      size_t tspace = jack_ringbuffer_write_space(tport->rings[i]);
      if (tspace < space)
        space = tspace;
    }

    if (space < (nframes * sizeof(float))) {
      unsigned int orun = nframes - (space / sizeof(float));
      tport->overruns += orun;
    }
    space = std::min(space, nframes * sizeof(float));

    // transfer samples to ringbuffer
    // there should always be enough space
    for (chn = 0; chn < tport->channels; chn++) {
      jack_ringbuffer_write (tport->rings[chn], (const char *) (tport->in[chn]),  space);
    }
    
    if (pthread_mutex_trylock (&(tport->meter_lock)) == 0) {
      pthread_cond_signal (&(tport->data_ready));
      pthread_mutex_unlock (&(tport->meter_lock));
    }
    return 0;
}

AlsaTPort::AlsaTPort(unsigned int card, unsigned int bits_per_sample, unsigned int sample_rate) {
  this->bits_per_sample = bits_per_sample;
  this->sample_rate = sample_rate;
  this->channels = 0;
  this->process_flag = 0;

  // set to 1 if threads should stop
  stop_flag = false;

  jack_status_t status;
  pthread_mutex_init(&meter_lock, NULL);
  pthread_cond_init(&data_ready, NULL);

  overruns = 0;

  if ((client = jack_client_open("james", JackNullOption, &status)) == 0) {
    printf("Failed to open jack client. %d \n", status);
  } 


  jack_set_process_callback(client, process, this);

  if( jack_activate(client) ) {
    fprintf(stderr, "can't activate client!");
  }

  const char **names = jack_get_ports(client, NULL, NULL, JackPortIsOutput);
  for(int i=0;names[i] != '\0';i++){
    channels++;
  }

  ports = (jack_port_t **) malloc (sizeof (jack_port_t *) * channels);

  for(int i=0;names[i] != '\0';i++){
    char name[64];
    sprintf (name, "input%d", i+1);
    if ((ports[i] = jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) == 0){
      printf("failed to register %s!\n", names[i]);
    }
    else
      printf("registered %s\n", name);
  }

  for(int i=0;i < channels;i++){
    if (jack_connect(client, names[i], jack_port_name(ports[i]))){
      printf("cannot connect port %s!\n", names[i]);
    }
    else
      printf("connected %s\n", names[i]);
  }   

  rings = (jack_ringbuffer_t**)malloc(sizeof(jack_ringbuffer_t*) * channels);
  for(int i = 0;i < channels;i++) {
    rings[i] = jack_ringbuffer_create(sizeof(float) * sample_rate * RING_LENGTH);
    jack_ringbuffer_mlock(rings[i]);
  }



  prerollSize = PREROLL_LENGTH * 10;
#ifdef DEBUG
  printf("prerollbuffer: %d\n", prerollSize);
#endif

  
  spool = new Spool(prerollSize, (sample_rate  / UPDATE_INTERVAL) * sizeof(FLAC__int32) * channels, this->bits_per_sample, this->sample_rate);
  meter = new Meter(channels, sample_rate, rings, spool, meter_lock, data_ready);

  process_flag = 1;
#ifdef DEBUG
  printf("ready...\n");
#endif
}

AlsaTPort::~AlsaTPort() {
  delete(meter);
  delete(spool);
  free(ports);
  for(int i = 0;i < channels; i++)
    jack_ringbuffer_free(rings[i]);
  free(rings);
}

void AlsaTPort::startRecording(char *path) {
  spool->start(path);
}

void AlsaTPort::stopRecording() {
  Spool *oldSpool = spool;
  spool = new Spool(prerollSize, (sample_rate  / UPDATE_INTERVAL) * sizeof(FLAC__int32) * channels, this->bits_per_sample, this->sample_rate);
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
