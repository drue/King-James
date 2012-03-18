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
  for(unsigned i=0;i < chans;i++)
    max[i] = prev[i] = 0;
 
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
  unsigned is, os;
  FLAC__int32 tMax[obj->chans];
  unsigned int frames;
  zmq::context_t ctx(1);
  zmq::socket_t socket(ctx, ZMQ_PUB);
  unsigned x;
  
  socket.bind("ipc:///tmp/peaks.ipc");

  jack_ringbuffer_data_t regions[2];
  QItem *o = obj->spool->getEmpty();

  for(unsigned i=0;i<obj->chans;i++)
    tMax[i] = 0;

  pthread_mutex_lock(obj->lock);
  printf("meter running\n");

  while(1) {
    pthread_cond_wait (obj->cond, obj->lock);
    size_t size = jack_ringbuffer_read_space(obj->ring);
    frames = size / 4 / obj->chans;
    
    jack_ringbuffer_get_read_vector(obj->ring, regions);

    for(unsigned i=0;i < obj->chans;i++)
      tMax[i] = 0;

    is = os = 0;
    for(unsigned i=0;i < frames;i++) {
      if (o->size == o->bufsize) {
        QItem *z = o;
        o = obj->spool->getEmpty();
        obj->shipItem(z, tMax, &socket);
        os = 0;
        for(unsigned i=0;i<obj->chans;i++)
          tMax[i] = 0;
      }
      for(unsigned c=0;c < obj->chans;c++) {
        int n;
        if(is < regions[0].len) {
          n = 0;
          x = is;
        }
        else {
          n = 1;
          x = is - regions[0].len;
        }
        o->buf[os / 4] = (FLAC__int32)*(regions[n].buf + x);
        if (abs(o->buf[os / 4]) > tMax[c]) 
          tMax[c] = abs(o->buf[os / 4]);
        os += 4;
        is += 4;
        o->size += 4;
      }
      if (o->size > o->bufsize) {
        printf("OVERFLOW\n");
        exit(-1);
      }
    }
    jack_ringbuffer_read_advance(obj->ring, is);
  }
  pthread_mutex_unlock(obj->lock);
}

void Meter::shipItem(QItem*item, FLAC__int32*tMax, zmq::socket_t *socket) {
  int millis;
  int decay;
  char str[1024] = "";
  char tmp[64];

  millis = item->size / 4 / chans / rate * 1000;
  decay = 8388608 * millis / DECAY;

  pthread_mutex_lock(&maxLock);	
  for(unsigned i=0;i < chans;i++) {
    if (tMax[i] > max[i]) 
      max[i] = tMax[i];
    if(tMax[i] < (prev[i] - decay))
        tMax[i] = prev[i] - decay;
      prev[i] = tMax[i];
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
