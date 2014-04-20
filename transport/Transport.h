#include <alsa/asoundlib.h>

#include <pthread.h>
#include <semaphore.h>

#include <jack/ringbuffer.h>

#include "port.h"
#include "spool.h"
#include "meter.h"

class AlsaTPort {
 public:
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
  bool spawns;

  snd_pcm_t *handle;
  snd_pcm_format_t format;
  snd_pcm_uframes_t period_time;
  snd_output_t *log;

  int bits_per_sample, sample_rate, channels;
  unsigned int update_interval;
  unsigned int ring_length;
  int process_flag;
  sem_t finished_sem;
  AlsaTPort(const char *card, unsigned int bits_per_sample = 24, unsigned int sample_rate = 48000, unsigned int update_interval = 5, unsigned int ring_length = 0, bool run = true);
  virtual ~AlsaTPort();
  void startRecording(char *path);
  void stopRecording();
  virtual void stop();
  virtual void wait();
  virtual void tick(snd_pcm_sframes_t (*reader)(snd_pcm_t *handle, void *buf, snd_pcm_uframes_t frames));
  virtual int gotSignal();
  long getmaxn(unsigned int n);
  void resetmax();
};


