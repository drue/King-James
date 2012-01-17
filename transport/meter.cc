#include <pthread.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include <zmq.hpp>

#include "meter.h"
#include "memq.h"

#define DECAY 1700

Meter::Meter(MemQ *aQ, Spool *aSpool) {
  finished = false;
  Q = aQ;
  spool = aSpool;
  amax = bmax = 0;

  pthread_mutex_init(&maxLock, NULL);
  pthread_mutex_init(&spoolLock, NULL);

  pthread_create(&sthread, NULL, (void * (*)(void *))run, this);
 
}

void Meter::switchSpool(Spool *newSpool) {
  spool = newSpool;
}

long Meter::getmaxa() 
{
  long ret;
  pthread_mutex_lock(&maxLock);
  ret = amax;
  pthread_mutex_unlock(&maxLock);
  return ret;
}

long Meter::getmaxb() 
{
  long ret;
  pthread_mutex_lock(&maxLock);
  ret = bmax;
  pthread_mutex_unlock(&maxLock);
  return ret;
}

void  Meter::resetmax() 
{
  pthread_mutex_lock(&maxLock);
  amax = bmax = 0;
  pthread_mutex_unlock(&maxLock);
}

float todBFS(FLAC__int32 sample)  {
  return 20 * log10(sample / 8388607.0);
}

void Meter::run(void *foo) {
  Meter *obj = (Meter *)foo;
  unsigned ws;
  FLAC__int32 tMaxA, tMaxB;
  QItem *i;
  zmq::context_t ctx(1);
  zmq::socket_t socket(ctx, ZMQ_PUB);
  struct timespec ts;

  socket.bind("ipc:///tmp/peaks.ipc");

  char str[256];
  int err;
  int millis;
  int decay;
  FLAC__int32 prevA = 0;
  FLAC__int32 prevB = 0;

  while(1) {
    // get QItem from Q
    clock_gettime(CLOCK_REALTIME, &ts) ;
    ts.tv_nsec += 100000000;
    i = obj->Q->getHead(&ts);
    if (i != NULL) {
    
      ws = tMaxA = tMaxB = 0;
    
      while (ws < i->frames * 2) {
        // find peaks
        if (abs(i->buf[ws]) > tMaxA) tMaxA = abs(i->buf[ws]);
        if (abs(i->buf[ws+1]) > tMaxB) tMaxB = abs(i->buf[ws+1]);
        ws+= 2;
      }
    
      millis = i->frames * 1000 / 46500;
      decay = 8388608 * millis / DECAY;

      pthread_mutex_lock(&obj->maxLock);	
      if (tMaxA > obj->amax) obj->amax = tMaxA;
      if (tMaxB > obj->bmax) obj->bmax = tMaxB;

      if(tMaxA < (prevA - decay))
        tMaxA = prevA - decay;
      if(tMaxB < (prevB - decay))
        tMaxB = prevB - decay;

      prevA = tMaxA;
      prevB = tMaxB;

      sprintf(str, "[%.0f, %.0f, %.0f, %.0f]", todBFS(tMaxA),  todBFS(tMaxB), todBFS(obj->amax),  todBFS(obj->bmax));
      pthread_mutex_unlock(&obj->maxLock);
      
      obj->spool->pushItem(i);

      zmq::message_t msg(str, strlen(str), NULL);
      socket.send(msg);
    }
  }
}
