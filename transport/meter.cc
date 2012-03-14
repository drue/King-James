#include <pthread.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <algorithm>

#include <zmq.hpp>

#include "meter.h"
#include "memq.h"

#define DECAY 1700

Meter::Meter(unsigned int chans, unsigned int sample_rate, jack_ringbuffer_t **qs, Spool *aSpool, pthread_mutex_t l, pthread_cond_t c) {
  finished = false;
  rings = qs;
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
  return 20 * log10(sample / 8388607.0);
}

void Meter::run(void *foo) {
  Meter *obj = (Meter *)foo;
  unsigned is;
  FLAC__int32 tMax[obj->chans];
  unsigned int frames;
  unsigned int ws;
  zmq::context_t ctx(1);
  zmq::socket_t socket(ctx, ZMQ_PUB);
  struct timespec ts;

  
  socket.bind("ipc:///tmp/peaks.ipc");

  jack_ringbuffer_data_t regions[obj->chans][2];
  QItem *o = obj->spool->getEmpty();

  for(unsigned i=0;i<obj->chans;i++)
    tMax[i] = 0;

  pthread_mutex_lock(&obj->lock);

  while(1) {
    clock_gettime(CLOCK_REALTIME, &ts) ;
    ts.tv_nsec += 100000000;
    jack_ringbuffer_get_read_vector(obj->rings[0], regions[0]);
    size_t size = regions[0][0].len + regions[0][1].len;
    for(unsigned i =1; i < obj->chans;i++) {
      jack_ringbuffer_get_read_vector(obj->rings[i], regions[i]);
      size = std::min(size, regions[i][0].len + regions[i][1].len);
    }

    frames = size / sizeof(float);

    for(unsigned i=0;i < obj->chans;i++)
      tMax[i] = 0;

    is = 0;
    for(unsigned i=0;i < frames;i++) {
      for(unsigned c=0;c < obj->chans;c++) {
        int n;
        ws = o->frames * obj->chans * sizeof(float);
        if(is < regions[c][0].len)
          n = 0;
        else
          n = 1;
        o->buf[ws] = (FLAC__int32)((float)*(regions[c][n].buf + is) * 8388607); //xxx

        if (abs(o->buf[ws]) > tMax[c]) 
          tMax[c] = abs(o->buf[ws]);
        ws += sizeof(float);
        is += sizeof(float);
      }

      o->frames += 1;

      if (o->frames * obj->chans * sizeof(float) == o->bufsize) {
        obj->shipItem(o, tMax, &socket);
        o = obj->spool->getEmpty();
        for(unsigned i=0;i<obj->chans;i++)
          tMax[i] = 0;
      }    
    }
    pthread_cond_wait (&obj->cond, &obj->lock);
  }
  pthread_mutex_unlock(&obj->lock);
}

void Meter::shipItem(QItem*item, FLAC__int32*tMax, zmq::socket_t *socket) {
  int millis;
  int decay;
  char str[1024] = "";
  char tmp[64];

  millis = item->frames * 1000 / rate;
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
  pthread_mutex_unlock(&maxLock);
  
  spool->pushItem(item);
  
  zmq::message_t msg(str, strlen(str), NULL);
  socket->send(msg);
}
