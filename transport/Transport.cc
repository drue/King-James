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
#define CHANS 2
#define UPDATE_INTERVAL 10 // hz

void Transport::stop() {
  // stop playing
}

void Transport::startRecording(char *path){
  // start recording
}


void AlsaTPort::stop() {
  stop_flag = true;
}

// this function is called by jack and should be at realtime priority
// it should do as little as possible as quickly as possible because nothing
// else happens while it is running
int AlsaTPort::process(jack_nframes_t nframes, void *user)
{
	int chn;
	size_t space = 0;
	AlsaTPort *tport = (AlsaTPort *) user;

    // get buffers
	for (chn = 0; chn < CHANS; chn++)
      tport->in[chn] = (jack_default_audio_sample_t*) jack_port_get_buffer (tport->ports[chn], nframes);
    
    // we have to write the same amount to every ring, otherwise the channels could get out of sync if
    // there is an overflow on only some channels because the drain is halfway through the rings
    // the ring should be large enough to always have enough space unless there is a problem
    space = jack_ringbuffer_write_space(tport->rings[0]);    
    for (int i=1;i<CHANS;i++) {
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
    for (chn = 0; chn < CHANS; chn++) {
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
        
  // set to 1 if threads should stop
  stop_flag = false;

  jack_status_t status;
  pthread_mutex_init(&meter_lock, NULL);
  pthread_cond_init(&data_ready, NULL);

  overruns = 0;

  rings = (jack_ringbuffer_t**)malloc(sizeof(jack_ringbuffer_t*) * CHANS);
  for(int i = 0;i < CHANS;i++) {
    rings[i] = jack_ringbuffer_create(sizeof(float) * sample_rate * RING_LENGTH);
    jack_ringbuffer_mlock(rings[i]);
  }

  if ((client = jack_client_open("james", JackNullOption, &status)) == 0) {
    printf("Failed to open jack client.\n");
  } 

  ports = (jack_port_t **) malloc (sizeof (jack_port_t *) * CHANS);


  prerollSize = PREROLL_LENGTH * 10;
#ifdef DEBUG
  printf("prerollbuffer: %d\n", prerollSize);
#endif

  
  spool = new Spool(prerollSize, (sample_rate  / UPDATE_INTERVAL) * sizeof(FLAC__int32) * CHANS, this->bits_per_sample, this->sample_rate);
  meter = new Meter(CHANS, sample_rate, rings, spool, meter_lock, data_ready);

#ifdef DEBUG
  printf("Initialized...\n");
#endif

  // create capture thread
  pthread_create(&cthread, NULL, (void * (*)(void *))doCapture, this);

#ifdef DEBUG
  printf("Threads launched...\n");
#endif


}

AlsaTPort::~AlsaTPort() {
  delete(meter);
  delete(spool);
  free(ports);
  for(int i = 0;i < CHANS; i++)
    jack_ringbuffer_free(rings[i]);
  free(rings);
}

void AlsaTPort::startRecording(char *path) {
  spool->start(path);
}

void AlsaTPort::stopRecording() {
  Spool *oldSpool = spool;
  spool = new Spool(prerollSize, (sample_rate  / UPDATE_INTERVAL) * sizeof(FLAC__int32) * CHANS, this->bits_per_sample, this->sample_rate);
  meter->switchSpool(spool);
  oldSpool->finish();
}

/*
void AlsaTPort::doCapture(void *foo)
{
  AlsaTPort *obj = (AlsaTPort *)foo;
  QItem *i;
  struct sched_param schp;
  int size = 0;
  const int bits_per_frame = obj->cap->getBitsPerFrame();
    
#ifdef DEBUG
  printf("capture thread starting\n");
#endif
    
  //usleep(100);
  // get real-time priority, this will only work as root!
  memset(&schp, 0, sizeof(schp));
  schp.sched_priority = sched_get_priority_max(SCHED_FIFO);

  if (sched_setscheduler(0, SCHED_FIFO, &schp) != 0) {
    perror("sched_setscheduler");
  }
  
  //obj->cap->monitorOn();
  obj->cap->prepare();
  obj->cap->start();

#ifdef DEBUG
  time_t t = time(NULL);
  printf("starting at %s\n", asctime(localtime(&t)));
  printf("bpf=%d pt=%d rt=%d\n", bits_per_frame, obj->cap->getPeriodTime(), bits_per_frame >> 3);
#endif
    
  
  while(!obj->stop_flag) {
    // get empty QItem
    i = obj->Q->getEmpty();
    // pcm_read into MItem->buf
    size = obj->cap->readIntoBuf(i->buf, i->bufsize / (4 * 2));
    if (size > 0){
      // push MItem onto Q
      i->frames = size;
      obj->Q->putTail(i);
    }
    else if (size == 0) {
      obj->Q->returnEmpty(i);
      continue;
    }
    else if (size == -1) {
      obj->Q->returnEmpty(i);
      obj->cap->stop();
      // no signal
      usleep(1000);
      // try to start again...
      obj->cap->start();
    }
    else {
      obj->Q->returnEmpty(i);
      // fatal
      break;
    }
  }
  obj->cap->stop();
  sem_post(&obj->finished_sem);
}
*/

void AlsaTPort::wait()
{
  int n = 0;
  while(n < 1) {
    sem_wait(&finished_sem);
    n++;
  }
}

int AlsaTPort::gotSignal() {
  return false;
}
void AlsaTPort::doReturn()
{
}

long AlsaTPort::getmaxn(unsigned int n) 
{
  return meter->getmaxn(n);
}

void  AlsaTPort::resetmax() 
{
  meter->resetmax();
}
