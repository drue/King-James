/*
 *  Transport.cc
 *  
 *
 *  Created on Mon Oct 01 2001.
 *  Copyright (c) 2001-2010 Andrew Loewenstern. All rights reserved.
 *
 */

#include "Transport.h"
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <FLAC/all.h>

#include "const.h"

#define NUM_PERIODS 8
#define DISK_BUFFER_SIZE BLOCK_SIZE * 64

#define PREROLL_LENGTH 30

void Transport::stop() {
  // stop playing
}

void Transport::startRecording(char *path){
  // start recording
}


void AlsaTPort::stop() {
  stop_flag = 1;
}


AlsaTPort::AlsaTPort(unsigned int card, unsigned int bits_per_sample, unsigned int sample_rate) {
  this->bits_per_sample = bits_per_sample;
  this->sample_rate = sample_rate;
        
  // init mutexes and semaphores
  sem_init(&asem, 0, 0);
  sem_init(&finished_sem, 0, 0);
    
  // set to 1 if threads should stop
  stop_flag = 0;
  // tstop is for the writer thread, don't set it yourself
  tstop = 0;
  // set when we are all finished
  finished = 0;
     
  cap = new APort(0, card, bits_per_sample, sample_rate);  // 0 == capture

  this->aligned_buffer_size = cap->getPeriodBytes() * NUM_PERIODS;
#ifdef DEBUG
  printf("aligned buffer size: %d\n", aligned_buffer_size);
#endif
    
  Q = new MemQ((PREROLL_LENGTH * sample_rate * sizeof(FLAC__int32) * 2) / aligned_buffer_size, aligned_buffer_size);

  pthread_mutex_init(&maxlock, NULL);  // locks the main queue

#ifdef DEBUG
  printf("Initialized...\n");
#endif
}

AlsaTPort::~AlsaTPort() {
  delete(Q);
  delete(cap);
  delete(wbuffer);
  free(filename);
  sem_destroy(&asem);
  sem_destroy(&finished_sem);
}

void AlsaTPort::startRecording(char *path) {
  FLAC__StreamEncoderInitStatus initted;
    
  stop_flag = 0;
  tstop = 0;
  finished = 0;

  filename = (char *)malloc(strlen(path));
  strcpy(filename, path);

  output = fopen64(path, "w+");

  // only cache beginning of this huge file we're about to write out and not reuse
  posix_fadvise(fileno(output), 8192, 0, POSIX_FADV_NOREUSE);

#ifdef DEBUG
  printf("file open.\n");
#endif



  wbuffer = new QItem(DISK_BUFFER_SIZE);
  encoder = FLAC__stream_encoder_new();
  FLAC__stream_encoder_set_channels(encoder, 2);
  FLAC__stream_encoder_set_bits_per_sample(encoder, bits_per_sample);
  FLAC__stream_encoder_set_sample_rate(encoder, sample_rate);
  FLAC__stream_encoder_set_do_mid_side_stereo(encoder, 0);
  FLAC__stream_encoder_set_blocksize(encoder, 4608);
  FLAC__stream_encoder_set_max_lpc_order(encoder, 0);
  FLAC__stream_encoder_set_min_residual_partition_order(encoder, 3);
  FLAC__stream_encoder_set_max_residual_partition_order(encoder, 3);

  initted = FLAC__stream_encoder_init_FILE(encoder, output, NULL, this);
  if (initted != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
    {
      printf("Couldn't initialize FLAC encoder.\n");
      exit(-1);
    }

#ifdef DEBUG
  printf("initted = %d, %s\n", initted, *FLAC__StreamEncoderStateString);
  printf("FLAC:  %d ch : %d bps : %d sr\n", FLAC__stream_encoder_get_channels(encoder), FLAC__stream_encoder_get_bits_per_sample(encoder), FLAC__stream_encoder_get_sample_rate(encoder));
#endif

  amax = bmax = aavg = bavg = 0;

  // create capture thread
  pthread_create(&cthread, NULL, (void * (*)(void *))doCapture, this);
  // start write to disk
  pthread_create(&sthread, NULL, (void * (*)(void *))doSave, this);
#ifdef DEBUG
  printf("Threads launched...\n");
#endif
}

void AlsaTPort::doCapture(void *foo)
{
  AlsaTPort *obj = (AlsaTPort *)foo;
  QItem *i;
  struct sched_param schp;
  int size = 0;
  const int bits_per_frame = obj->cap->getBitsPerFrame();
    
#ifdef DEBUG
  printf("capture thread starting\n");
#endif
    
  usleep(100);
  // get real-time priority, this will only work as root!
  memset(&schp, 0, sizeof(schp));
  schp.sched_priority = sched_get_priority_max(SCHED_FIFO);

  if (sched_setscheduler(0, SCHED_FIFO, &schp) != 0) {
    perror("sched_setscheduler");
  }
  
  //obj->cap->monitorOn();
  obj->cap->prepare();
  obj->cap->start();

#ifdef DEBUG
  time_t t = time(NULL);
  printf("starting at %s\n", asctime(localtime(&t)));
  printf("bpf=%d pt=%d rt=%d\n", bits_per_frame, obj->cap->getPeriodTime(), bits_per_frame >> 3);
#endif
    
  while(!obj->stop_flag) {
    // get empty QItem
    i = obj->Q->getEmpty();
    // pcm_read into MItem->buf
    size = obj->cap->readIntoBuf(i->buf, i->bufsize / (4 * 2));
    if (size > 0){
      // push MItem onto Q
      i->frames = size;
      obj->Q->putTail(i);
      // signal semaphore
      sem_post(&(obj->asem));
    }
    else if (size == 0) {
      obj->Q->returnEmpty(i);
      continue;
    }
    else if (size == -1) {
      obj->Q->returnEmpty(i);
      obj->cap->stop();
      // no signal
      usleep(1000);
      // try to start again...
      obj->cap->start();
    }
    else {
      obj->Q->returnEmpty(i);
      // fatal
      break;
    }
  }
  printf("Capture Stopping...\n");
  obj->tstop = 1;
  sem_post(&(obj->asem));
  obj->cap->stop();
  //obj->cap->monitorOff();
}

void AlsaTPort::doSave(void *foo)
{
  AlsaTPort *obj = (AlsaTPort *)foo;
  QItem *i;
  int remaining, finished = 0;
  unsigned ws;
  FLAC__int32 tMaxA, tMaxB;

#ifdef DEBUG
  printf("Save thread starting...\n");
#endif

  do {
    // wait on semaphore
    sem_wait(&(obj->asem));
    // get QItem from Q
    i = obj->Q->getHead();
    if (i != NULL) {

      ws = tMaxA = tMaxB = 0;

      while (ws < i->frames * 2) {
        // find peaks
        if (abs(i->buf[ws]) > tMaxA) tMaxA = abs(i->buf[ws]);
        if (abs(i->buf[ws+1]) > tMaxB) tMaxB = abs(i->buf[ws+1]);
        ws+= 2;
      }
      
      pthread_mutex_lock(&obj->maxlock);	
      if (tMaxA > obj->amax) obj->amax = tMaxA;
      if (tMaxB > obj->bmax) obj->bmax = tMaxB;
      pthread_mutex_unlock(&obj->maxlock);

      // write QItem->buf to disk
      if (!FLAC__stream_encoder_process_interleaved(obj->encoder, (const FLAC__int32 *)i->buf, i->frames)){
        printf("FLAC error! state = %d:%s \n", FLAC__stream_encoder_get_state(obj->encoder), 
               FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(obj->encoder)]);
      }
      // put QItem back into pool
      obj->Q->returnEmpty(i);
      /*free(out[0]);
        free(out[1]);*/
    }
    // check if we're stopped
    if(obj->tstop == 1) {
      sem_getvalue(&(obj->asem), &remaining);
#ifdef DEBUG
      printf("remaining: %d\n", remaining);
#endif
      // stopped and no more 
      if(!remaining) {
        FLAC__stream_encoder_finish(obj->encoder);
        FLAC__stream_encoder_delete(obj->encoder);
        fclose(obj->output);
        ++finished;
      }
    }
  } while (finished == 0);
  obj->finished=1;
  sem_post(&(obj->finished_sem));  // all done
}

void AlsaTPort::wait()
{
  if(!finished)
    sem_wait(&finished_sem);
}

int AlsaTPort::gotSignal() {
  return cap->got_signal;
}
void AlsaTPort::doReturn()
{
}

long AlsaTPort::getmaxa() 
{
  long ret;
  pthread_mutex_lock(&maxlock);
  ret = amax;
  pthread_mutex_unlock(&maxlock);
  return ret;
}

long AlsaTPort::getmaxb() 
{
  long ret;
  pthread_mutex_lock(&maxlock);
  ret = bmax;
  pthread_mutex_unlock(&maxlock);
  return ret;
}

void  AlsaTPort::resetmax() 
{
  pthread_mutex_lock(&maxlock);
  amax = bmax = 0;
  pthread_mutex_unlock(&maxlock);
}

