#include <pthread.h>
#include <FLAC/all.h>
#include <jack/ringbuffer.h>
#include <zmq.hpp>

#include "spool.h"

class Meter {
 protected:
  pthread_t sthread;
  pthread_t cthread;
  bool finished;
  pthread_mutex_t *lock;
  pthread_cond_t *cond;
  unsigned int chans;
  unsigned int rate;
  jack_ringbuffer_t *ring;
  Spool *spool;
  pthread_mutex_t maxLock;
  FLAC__int32 *max;
  FLAC__int32 *prev;
  zmq::socket_t *socket;
  zmq::context_t *ctx;

  static void run(void *foo);
  void shipItem(buffer &i, FLAC__int32*tMax, zmq::socket_t *socket);

 public:
  virtual ~Meter();

  Meter(unsigned int chans, unsigned int sample_rate, jack_ringbuffer_t *q, Spool *aSpool, pthread_mutex_t *lock, pthread_cond_t *cond);
  long getmaxn(unsigned int n);
  virtual void tick();
  void start();
  void resetmax();
  void switchSpool(Spool *newSpool);
};
