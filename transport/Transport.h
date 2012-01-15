#include <alsa/asoundlib.h>

#include <pthread.h>
#include <semaphore.h>

#include "port.h"
#include "memq.h"
#include "spool.h"
#include "meter.h"


class Transport {
 public:
  virtual void stop();
  virtual void startRecording(char *path);
};

class AlsaTPort: public Transport {
 protected:
  MemQ *Q;
  Meter *meter;
  Spool *spool;

  bool stop_flag;
  int bits_per_sample, sample_rate;
  APort *pla;
  APort *cap;
  unsigned int prerollSize;
  unsigned int aligned_buffer_size;
  pthread_t cthread;
  static void doCapture(void *foo);
  void doReturn();

 public:
  sem_t finished_sem;
  AlsaTPort(unsigned int card, unsigned int bits_per_sample, unsigned int sample_rate);
  virtual ~AlsaTPort();
  void startRecording(char *path);
  void stopRecording();
  virtual void stop();
  virtual void wait();
  virtual int gotSignal();
  long getmaxa();
  long getmaxb();
  void resetmax();
    
};
