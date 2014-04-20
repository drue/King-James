#include <pthread.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <algorithm>

#include <zmq.hpp>

#include "meter.h"

#define DECAY 1700

Meter::Meter(unsigned int chans, unsigned int sample_rate, jack_ringbuffer_t *q, Spool *aSpool, pthread_mutex_t *l, pthread_cond_t *c, bool levels) {
  finished = false;
  ring = q;
  spool = aSpool;
  this->chans = chans;
  send_levels = levels;
  o = &spool->getEmpty();
  os = 0;

  max = (FLAC__int32*)malloc(sizeof(FLAC__int32*) * chans);
  prev = (FLAC__int32*)malloc(sizeof(FLAC__int32*) * chans);
  for(unsigned i=0;i < chans;i++) {
    max[i] = prev[i] = 0;
  }
  
  pthread_mutex_init(&maxLock, NULL);
  lock = l;
  cond = c;
  rate = sample_rate;
  ready = done = false;

  if (send_levels) {
    ctx = new zmq::context_t(1);
    socket = new zmq::socket_t(*ctx, ZMQ_PUB);
    socket->bind("ipc:///tmp/peaks.ipc");
  }
}

Meter::~Meter() {
  free (max);
  free (prev);
  if (send_levels) {
    delete(socket);
    delete(ctx);
  }
}

void Meter::start() {
  pthread_create(&sthread, NULL, (void * (*)(void *))run, this);
}

void Meter::finish() {
  boost::mutex::scoped_lock lock(readyLock);
  finished = true;
  pthread_cond_signal(cond);
}

void Meter::wait() {
  boost::mutex::scoped_lock lock(readyLock);
  while(!done)
    readyCond.wait(readyLock);
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

void Meter::waitReady() {
  boost::mutex::scoped_lock lock(readyLock);
  while(!ready)
    readyCond.wait(readyLock);
}

void Meter::run(void *foo) {
  bool exiting = false;
  Meter *obj = (Meter *)foo;
  
  obj->readyLock.lock();
  obj->ready = true;
  obj->readyCond.notify_all();
  obj->readyLock.unlock();


  do {
    pthread_mutex_lock(obj->lock);
    obj->tick();
    int s = jack_ringbuffer_read_space(obj->ring);
    if(obj->finished && s == 0) {
      exiting = true;
    }
    else if (s == 0) {
      pthread_cond_wait (obj->cond, obj->lock);
    }
    pthread_mutex_unlock(obj->lock);
  }  while(!exiting);
  boost::mutex::scoped_lock lock(obj->readyLock);
  obj->done = true;
  obj->readyCond.notify_all();
}

void Meter::tick() {
  size_t is, i, c;
  FLAC__int32 *tMax, *ibuf, t;
  size_t frames;
  size_t x;
  jack_ringbuffer_data_t regions[2];

  tMax = (FLAC__int32 *)malloc(sizeof(chans * sizeof(FLAC__int32)));

  for(i=0;i<chans;i++)
    tMax[i] = 0;

  jack_ringbuffer_get_read_vector(ring, regions);
  frames = (regions[0].len + regions[1].len)  / 4 / chans;

  is = 0;

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
        if (t > 8388608) {
          t = 16777216 - t;
        }
        if (t > tMax[c]) {
          tMax[c] = t;
        }
        os += 4;
        is += 4;
      }
      if (o->size == os) {
        shipItem(*o, tMax);
        for(i=0;i<chans;i++)
          tMax[i] = 0;
        os = 0;
        o = &spool->getEmpty();
      }
      else if (os > o->size) {
        printf("OVERFLOW\n");
      }
    }
    jack_ringbuffer_read_advance(ring, is);
  }
  free(tMax);
}
void Meter::shipItem(buffer &item, FLAC__int32*tMax) {
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
  if (send_levels) {
    zmq::message_t msg(str, strlen(str), NULL);
    socket->send(msg);
  }
}
