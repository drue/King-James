#include <pthread.h>
#include <FLAC/all.h>
#include <jack/ringbuffer.h>
#include <zmq.hpp>

#include "spool.h"
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition.hpp"

class Meter {
 protected:
  pthread_t sthread;
  pthread_t cthread;
  bool send_levels;
  pthread_mutex_t *lock;
  pthread_cond_t *cond;

  bool ready;
  bool finished;
  bool done;
  boost::mutex readyLock;
  boost::condition readyCond;

  
  unsigned int chans;
  unsigned int rate;
  jack_ringbuffer_t *ring;
  Spool *spool;
  buffer *o;
  unsigned os;
  pthread_mutex_t maxLock;
  FLAC__int32 *max;
  FLAC__int32 *prev;
  zmq::socket_t *socket;
  zmq::context_t *ctx;

  static void run(void *foo);
  void shipItem(buffer &i, FLAC__int32*tMax);

 public:
  virtual ~Meter();

  Meter(unsigned int chans, unsigned int sample_rate, jack_ringbuffer_t *q, Spool *aSpool, pthread_mutex_t *lock, pthread_cond_t *cond, bool levels=true);
  long getmaxn(unsigned int n);
  virtual void tick();
  void waitReady();
  void start();
  void finish();
  void wait();
  void resetmax();
  void switchSpool(Spool *newSpool);
};
