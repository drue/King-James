#include <pthread.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <algorithm>

#include <zmq.hpp>

#include "meter.h"

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

  ctx = new zmq::context_t(1);
  socket = new zmq::socket_t(*ctx, ZMQ_PUB);
  socket->bind("ipc:///tmp/peaks.ipc");
}

Meter::~Meter() {
  free (max);
  free (prev);
  delete(socket);
  delete(ctx);
}

void Meter::start() {
  pthread_create(&sthread, NULL, (void * (*)(void *))run, this);
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
  
  pthread_mutex_lock(obj->lock);

  while(1) {
    //printf("awake\n");
    obj->tick();
    pthread_cond_wait (obj->cond, obj->lock);
  }
  pthread_mutex_unlock(obj->lock);
}

void Meter::tick() {
  unsigned is, os, i, c;
  FLAC__int32 *tMax, *ibuf, t;
  unsigned int frames;
  unsigned x;
  jack_ringbuffer_data_t regions[2];

  tMax = (FLAC__int32 *)malloc(sizeof(chans * sizeof(FLAC__int32)));

  buffer *o = &spool->getEmpty();
  for(i=0;i<chans;i++)
    tMax[i] = 0;

  size_t size = jack_ringbuffer_read_space(ring);
    
  jack_ringbuffer_get_read_vector(ring, regions);
  frames = (regions[0].len + regions[1].len)  / 4 / chans;

  is = 0;
  os = 0;

  if (frames > 0) {
    for(unsigned frame=0;frame < frames;frame++) {
      for(c=0;c < chans;c++) {
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
      }
      if (o->size == os) {
        shipItem(*o, tMax, socket);
        os = 0;
        o = &spool->getEmpty();
        //printf("shipped, freespace: %d\n", jack_ringbuffer_write_space(ring));
      }
      else if (os > o->size) {
        printf("OVERFLOW\n");
        exit(-1);
      }
    }
    jack_ringbuffer_read_advance(ring, is);
  }
  free(tMax);
}
void Meter::shipItem(buffer &item, FLAC__int32*tMax, zmq::socket_t *socket) {
  int millis;
  int decay;
  char str[1024] = "";
  char tmp[64];
  FLAC__int32 x;

  millis = item.size / 4 / chans / rate * 1000;
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
  //printf("%s\n", str);
  zmq::message_t msg(str, strlen(str), NULL);
  socket->send(msg);
}
