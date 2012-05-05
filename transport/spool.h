#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#include <zmq.hpp>

#include "memq.h"

#ifndef __SPOOL__
#define __SPOOL__

class Spool {
 protected:
  pthread_t sthread;;
  static void doWrite(void *foo);
  zmq::context_t ctx;
  zmq::socket_t socket;
  long long oFrames; // output frames
  long long aFrames; // all frames seen
  long long lastProgress; // aFrame we last sent progress on
  char progMsg[256];

 public:

  Spool(unsigned int prerollSize, unsigned int bufSize, unsigned int bps, unsigned int sr, unsigned int channels);
  ~Spool();
  char *filename;
  unsigned int bits_per_sample, sample_rate, channels;
  MemQ *Q;
  bool finished;
  bool started;
  pthread_mutex_t frameLock;

  virtual void pushItem(QItem *item);
  virtual QItem *getEmpty();
  virtual void start(char *savePath);
  virtual void finish();
};

#endif
