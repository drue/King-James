#include <deque>

#include <time.h>

#include <FLAC/all.h>
#include <zmq.hpp>
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition.hpp"
#include "boost/thread/thread.hpp"

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
  static void doWrite(void *foo);
  zmq::context_t *ctx;
  zmq::socket_t *socket;
  bool should_spawn;
  bool send_progress;
  long long oFrames; // output frames
  long long aFrames; // all frames seen
  long long lastProgress; // aFrame we last sent progress on
  char progMsg[256];
  FLAC__StreamEncoder *encoder;
  FILE *output;
  bool started; // flag indicating spool should start actually writing data to disk
  bool finished; // flag indicating thread should finish up and move to done
  bool ready; // thread running and processing data
  bool done; // thread done, flac finalized
  boost::mutex qLock;
  boost::condition finishCond;
  boost::condition dataCond;
  boost::thread *thread;
 public:

  Spool(unsigned int prerollSize, unsigned int bufSize, unsigned int bps, unsigned int sr, unsigned int channels, bool spawn=true, bool progress = true);
  ~Spool();
  char *filename;
  unsigned int bits_per_sample, sample_rate, channels, bufferSize, maxQSize;
  std::deque<buffer>Q;

  void initFLAC();
  void finishFLAC();
  void pushItem(buffer &item);
  buffer &getEmpty();
  void start(char *savePath);
  virtual int tick();
  void finish();
  void wait();
  void waitReady();
};

#endif
