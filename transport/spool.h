#include <deque>

#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#include <FLAC/all.h>
#include <zmq.hpp>

#ifndef __SPOOL__
#define __SPOOL__

class buffer {
 public:
  buffer(unsigned int bsize);
  ~buffer();
  int *buf;
  unsigned int size;
};

class Spool {
 protected:
  pthread_t sthread;;
  static void doWrite(void *foo);
  zmq::context_t ctx;
  zmq::socket_t socket;
  bool should_spawn;
  long long oFrames; // output frames
  long long aFrames; // all frames seen
  long long lastProgress; // aFrame we last sent progress on
  char progMsg[256];
  FLAC__StreamEncoder *encoder;
  FILE *output;


 public:

  Spool(unsigned int prerollSize, unsigned int bufSize, unsigned int bps, unsigned int sr, unsigned int channels, bool spawn=true);
  ~Spool();
  char *filename;
  unsigned int bits_per_sample, sample_rate, channels, bufferSize, maxQSize;
  std::deque<buffer>Q;
  bool finished;
  bool started;
  pthread_mutex_t qLock;
  pthread_mutex_t finishLock;
  pthread_cond_t finishCond;

  void initFLAC();
  void finishFLAC();
  void pushItem(buffer &item);
  buffer &getEmpty();
  void start(char *savePath);
  virtual int tick();
  void finish();
  void wait();
};

#endif
