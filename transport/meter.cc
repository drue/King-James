#include <pthread.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <algorithm>

#include <zmq.hpp>

#include "meter.h"
#include "memq.h"

#define DECAY 1700

Meter::Meter(unsigned int chans, unsigned int sample_rate, jack_ringbuffer_t *q, Spool *aSpool, pthread_mutex_t *l, pthread_cond_t *c) {
  finished = false;
  ring = q;
  spool = aSpool;
  this->chans = chans;
  max = (FLAC__int32*)malloc(sizeof(FLAC__int32*) * chans);
  prev = (FLAC__int32*)malloc(sizeof(FLAC__int32*) * chans);
  for(unsigned i=0;i < chans;i++) {
    max[i] = prev[i] = 0;
  }

  pthread_mutex_init(&maxLock, NULL);
  lock = l;
  cond = c;
  rate = sample_rate;

  pthread_create(&sthread, NULL, (void * (*)(void *))run, this);
  
}

Meter::~Meter() {
  free (max);
  free (prev);
}

void Meter::switchSpool(Spool *newSpool) {
  spool = newSpool;
}

long Meter::getmaxn(unsigned int n) 
{
  long ret;
  pthread_mutex_lock(&maxLock);
  ret = max[n];
  pthread_mutex_unlock(&maxLock);
  return ret;
}

void  Meter::resetmax() 
{
  pthread_mutex_lock(&maxLock);
  for(unsigned i=0;i<chans;i++) {
    max[i] = 0;
  }
  pthread_mutex_unlock(&maxLock);
}

float todBFS(FLAC__int32 sample)  {
  if(sample)
    return 20 * log10(sample / 8388607.0);
  else return -140;
}

void Meter::run(void *foo) {
  Meter *obj = (Meter *)foo;
  unsigned is, os, i, c;
  FLAC__int32 *tMax, *ibuf, t;
  unsigned int frames;
  zmq::context_t ctx(1);
  zmq::socket_t socket(ctx, ZMQ_PUB);
  unsigned x;
  
  socket.bind("ipc:///tmp/peaks.ipc");
  
  tMax = (FLAC__int32 *)malloc(sizeof(obj->chans * sizeof(FLAC__int32)));

  jack_ringbuffer_data_t regions[2];
  QItem *o = obj->spool->getEmpty();

  for(i=0;i<obj->chans;i++)
    tMax[i] = 0;

  pthread_mutex_lock(obj->lock);
  printf("meter running\n");

  while(1) {
    pthread_cond_wait (obj->cond, obj->lock);
    size_t size = jack_ringbuffer_read_space(obj->ring);
    
    jack_ringbuffer_get_read_vector(obj->ring, regions);
    frames = (regions[0].len + regions[1].len)  / 4 / obj->chans;

    is = 0;
    os = o->size;

    for(unsigned frame=0;frame < frames;frame++) {
      for(c=0;c < obj->chans;c++) {
        if(is < regions[0].len) {
          x = is;
          ibuf = (FLAC__int32*)regions[0].buf;
        }
        else {
          x = is - regions[0].len;
          ibuf = (FLAC__int32*)regions[1].buf;
        }
        t = o->buf[os/4] = ibuf[x/4];
        if (abs(t) > tMax[c]) {
          tMax[c] = abs(t);
        }
        os += 4;
        is += 4;
        o->size += 4;
      }
      if (o->size == o->bufsize) {
        QItem *z = o;
        o = obj->spool->getEmpty();
        obj->shipItem(z, tMax, &socket);
        os = 0;
        for(unsigned z=0;z < obj->chans;z++) {
          tMax[z] = 0;
        }
      }
      else if (o->size > o->bufsize) {
        printf("OVERFLOW\n");
        exit(-1);
      }
    }
    jack_ringbuffer_read_advance(obj->ring, is);
  }
  pthread_mutex_unlock(obj->lock);
  free(tMax);
}

void Meter::shipItem(QItem*item, FLAC__int32*tMax, zmq::socket_t *socket) {
  int millis;
  int decay;
  char str[1024] = "";
  char tmp[64];
  FLAC__int32 x;

  millis = item->size / 4 / chans / rate * 1000;
  decay = 8388608 * millis / DECAY;

  pthread_mutex_lock(&maxLock);	
  for(unsigned i=0;i < chans;i++) {
    if (tMax[i] > max[i]) 
      max[i] = tMax[i];
    if(tMax[i] < (prev[i] - decay)) {
      x = prev[i] - decay;
      prev[i] = x;
    }
    else {
      prev[i] = tMax[i];
    }
  }
  
  strcat(str, "[");
  for(unsigned i=0;i< chans;i++) {
    sprintf(tmp, "%.0f,", todBFS(tMax[i]));
    strcat(str, tmp);
  }
  for(unsigned i=0;i< chans;i++) {
    sprintf(tmp, "%.0f", todBFS(max[i]));
    strcat(str, tmp);
    if (i + 1 < chans)
      strcat(str, ",");
  }
  strcat(str, "]");
  pthread_mutex_unlock(&maxLock);
  spool->pushItem(item);
  zmq::message_t msg(str, strlen(str), NULL);
  socket->send(msg);
}
