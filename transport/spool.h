#include <pthread.h>
#include <semaphore.h>

#include "memq.h"

#ifndef __SPOOL__
#define __SPOOL__

class Spool {
 protected:
  pthread_t sthread;;
  static void doWrite(void *foo);

 public:

  Spool(unsigned int prerollSize, unsigned int bps, unsigned int sr);
  ~Spool();
  char *filename;
  unsigned int bits_per_sample, sample_rate;
  MemQ *Q;
  bool finished;

  virtual void pushItem(QItem *item);
  virtual void start(char *savePath);
  virtual void finish();
};

#endif
