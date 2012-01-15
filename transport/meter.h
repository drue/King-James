#include <pthread.h>
#include <FLAC/all.h>

#include "spool.h"
#include "memq.h"

class Meter {
 protected:
  pthread_t sthread;
  static void run(void *foo);
  pthread_t cthread;
  bool finished;

 public:
  MemQ *Q;
  Spool *spool;
  pthread_mutex_t maxLock;
  pthread_mutex_t spoolLock;
  FLAC__int32 amax, bmax;

  Meter(MemQ *aQ, Spool *aSpool);
  long getmaxa();
  long getmaxb();
  void resetmax();
  void switchSpool(Spool *newSpool);
};
