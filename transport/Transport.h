#include <alsa/asoundlib.h>

#include <pthread.h>
#include <semaphore.h>

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
  static void* process(void *user);
  void xrun(void);
  void suspend(void);
  void doReturn();
  void setup();

  jack_ringbuffer_t *ring;
  unsigned int nports;
  pthread_mutex_t meter_lock;
  pthread_cond_t  data_ready;

  unsigned long overruns;

  snd_pcm_t *handle;
  snd_pcm_format_t format;
  snd_pcm_uframes_t period_time;
  snd_output_t *log;

 public:
  int bits_per_sample, sample_rate, channels;
  int process_flag;
  sem_t finished_sem;
  AlsaTPort(char *card, unsigned int bits_per_sample, unsigned int sample_rate);
  virtual ~AlsaTPort();
  void startRecording(char *path);
  void stopRecording();
  virtual void stop();
  virtual void wait();
  virtual int gotSignal();
  long getmaxn(unsigned int n);
  void resetmax();
    
};


