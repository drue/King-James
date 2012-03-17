#include <alsa/asoundlib.h>

#include <pthread.h>
#include <semaphore.h>

#include <jack/types.h>
#include <jack/ringbuffer.h>

#include "port.h"
#include "memq.h"
#include "spool.h"
#include "meter.h"


class AlsaTPort {
 protected:
  MemQ *Q;
  Meter *meter;
  Spool *spool;

  bool stop_flag;
  unsigned int prerollSize;
  pthread_t cthread;
  static int process(jack_nframes_t nframes, void *user);

  void doReturn();

  jack_client_t *client;
  jack_ringbuffer_t **rings;
  jack_default_audio_sample_t **in;
  unsigned int nports;
  jack_port_t **ports;
  pthread_mutex_t meter_lock;
  pthread_cond_t  data_ready;

  unsigned long overruns;

 public:
  int bits_per_sample, sample_rate, channels;
  int process_flag;
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


