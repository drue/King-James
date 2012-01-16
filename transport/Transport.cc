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
#include <sys/stat.h>

#include <fcntl.h>
#include <FLAC/all.h>

#include "const.h"

#define NUM_PERIODS 8
#define DISK_BUFFER_SIZE BLOCK_SIZE * 64

#define PREROLL_LENGTH 60

void Transport::stop() {
  // stop playing
}

void Transport::startRecording(char *path){
  // start recording
}


void AlsaTPort::stop() {
  stop_flag = true;
}


AlsaTPort::AlsaTPort(unsigned int card, unsigned int bits_per_sample, unsigned int sample_rate) {
  this->bits_per_sample = bits_per_sample;
  this->sample_rate = sample_rate;
        
  // set to 1 if threads should stop
  stop_flag = false;
     
  cap = new APort(0, card, bits_per_sample, sample_rate);  // 0 == capture

  this->aligned_buffer_size = sample_rate / 10 * 8; // 1/10 of a second
#ifdef DEBUG
  printf("aligned buffer size: %d\n", aligned_buffer_size);
#endif
  
  prerollSize = PREROLL_LENGTH * 10;
#ifdef DEBUG
  printf("prerollbuffer: %d\n", prerollSize);
#endif

  Q = new MemQ(prerollSize, aligned_buffer_size);
  spool = new Spool(prerollSize, this->bits_per_sample, cap->sample_rate);
  meter = new Meter(Q, spool);

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
  delete(cap);
}

void AlsaTPort::startRecording(char *path) {
  spool->start(path);
}

void AlsaTPort::stopRecording() {
  Spool *oldSpool = spool;
  spool = new Spool(prerollSize, this->bits_per_sample, cap->sample_rate);
  meter->switchSpool(spool);
  oldSpool->finish();
}

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

void AlsaTPort::wait()
{
  int n = 0;
  while(n < 1) {
    sem_wait(&finished_sem);
    n++;
  }
}

int AlsaTPort::gotSignal() {
  return cap->got_signal;
}
void AlsaTPort::doReturn()
{
}

long AlsaTPort::getmaxa() 
{
  return meter->getmaxa();
}

long AlsaTPort::getmaxb() 
{
  return meter->getmaxb();
}

void  AlsaTPort::resetmax() 
{
  meter->resetmax();
}
