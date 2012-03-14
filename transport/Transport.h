#include <alsa/asoundlib.h>

#include <pthread.h>
#include <semaphore.h>

#include <jack/types.h>
#include <jack/ringbuffer.h>

#include "port.h"
#include "memq.h"
#include "spool.h"
#include "meter.h"


class Transport {
 public:
  virtual void startRecording(char *path);
  // just stop recording to above file
  virtual void stopRecording();
  // stop everything, including preroll
  virtual void stop();
  virtual void wait();

  long getmaxa();
  long getmaxb();
  void resetmax();
};

class AlsaTPort: public Transport {
 protected:
  MemQ *Q;
  Meter *meter;
  Spool *spool;

  bool stop_flag;
  int bits_per_sample, sample_rate;
  unsigned int prerollSize;
  pthread_t cthread;
  static void doCapture(void *foo);
  static int process(jack_nframes_t nframes, void *user);

  void doReturn();

  jack_client_t *client;
  jack_port_t **ports;
  jack_ringbuffer_t **rings;
  jack_default_audio_sample_t **in;
  pthread_mutex_t meter_lock;
  pthread_cond_t  data_ready;

  unsigned long overruns;

 public:
  sem_t finished_sem;
  AlsaTPort(unsigned int card, unsigned int bits_per_sample, unsigned int sample_rate);
  virtual ~AlsaTPort();
  void startRecording(char *path);
  void stopRecording();
  virtual void stop();
  virtual void wait();
  virtual int gotSignal();
  long getmaxn(unsigned int n);
  void resetmax();
    
};


