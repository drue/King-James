/*
 *  Transport.cc
 *  
 *
 *  Created by burris on Mon Oct 01 2001.
 *  Copyright (c) 2001-2010 Andrew Loewenstern. All rights reserved.
 *
 */

#include "Transport.h"
#include <alsa/asoundlib.h>
#include <pthread.h>
#include "core.h"
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <FLAC/stream_encoder.h>
#define NUM_PERIODS 8
#define DISK_BUFFER_SIZE BLOCK_SIZE * 64

void Transport::stop() {
  // stop playing
}

void Transport::startRecording(char *path){
  // start recording
}


void AlsaTPort::stop() {
  stop_flag = 1;
}


AlsaTPort::AlsaTPort(int bits_per_sample, int sample_rate) {
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
     
  cap = new APort(0, bits_per_sample, sample_rate);  // 0 == capture

  this->aligned_buffer_size = cap->getPeriodBytes() * NUM_PERIODS;
#ifdef DEBUG
  printf("aligned buffer size: %d\n", aligned_buffer_size);
#endif
    
  Q = new MemQ(2, aligned_buffer_size);

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
  FLAC__StreamEncoderState err;
  FLAC__StreamEncoderInitStatus initted;
    
  stop_flag = 0;
  tstop = 0;
  finished = 0;

  filename = (char *)malloc(strlen(path));
  strcpy(filename, path);

  fd = open64(path, O_WRONLY | O_TRUNC | O_CREAT | O_DIRECT, 
              S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    
  fchmod(fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
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

  initted = FLAC__stream_encoder_init_stream(encoder, write_callback, NULL, NULL, metadata_callback, this);

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
  MItem *i;
  struct sched_param schp;
  int size = 0;
  int r;
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
    // get empty MItem
    i = obj->Q->getEmpty();
    // pcm_read into MItem->buf
    size = obj->cap->readIntoBuf(i->bufs, i->maxsize / (bits_per_frame / 16));
    if (size > 0){
      // push MItem onto Q
      i->size = size;
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
      r = obj->cap->start();
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
  MItem *o, *i;
  int remaining, finished = 0;
  long x, *ias, *ibs;
  unsigned ws;
  FLAC__int32 *as, *bs;
#ifdef DEBUG
  printf("Save thread starting...\n");
#endif

  do {
    // wait on semaphore
    sem_wait(&(obj->asem));
    // get QItem from Q
    i = obj->Q->getHead();
    if (i != NULL) {

      o = obj->Q->getEmpty();
      x = 0;
      ws = 0;
      ias = (long*)o->bufs[0];
      ibs = (long*)o->bufs[1];
      as = (FLAC__int32*) o->bufs[0];
      bs = (FLAC__int32*) o->bufs[1];

      pthread_mutex_lock(&obj->maxlock);	
      while (ws < i->size) {

        as[ws] = i->bufs[0][x]; as[ws] <<= 8;
        as[ws] |= i->bufs[0][x+1]; as[ws] <<= 8;
        as[ws] |= i->bufs[0][x+2];

        bs[ws] = i->bufs[1][x]; bs[ws] <<= 8;
        bs[ws] |= i->bufs[1][x+1]; bs[ws] <<= 8;
        bs[ws] |= i->bufs[1][x+2];
	
        as[ws] -= 0x800000;
        bs[ws] -= 0x800000;

        if (ias[ws] > obj->amax) obj->amax = ias[ws];
        if (ibs[ws] > obj->bmax) obj->bmax = ibs[ws];

        ws++;
        x+=4;
      }
      pthread_mutex_unlock(&obj->maxlock);	

      // write QItem->buf to disk
      if (!FLAC__stream_encoder_process(obj->encoder, (const FLAC__int32 **)o->bufs, i->size)){
        printf("FLAC error! state = %d:%s \n", FLAC__stream_encoder_get_state(obj->encoder), 
               FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(obj->encoder)]);
      }
      // put QItem back into pool
      obj->Q->returnEmpty(i);
      obj->Q->returnEmpty(o);
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

FLAC__StreamEncoderWriteStatus AlsaTPort::write_callback(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
  int x = 0;
  unsigned int extras;
  QItem *toWrite = (QItem *)((AlsaTPort*)client_data)->wbuffer;
  int fd = (int)((AlsaTPort*)client_data)->fd;
    
  x = bytes;
  if( bytes > (toWrite->maxsize - toWrite->size) )
    x = toWrite->maxsize - toWrite->size;
    
  memcpy(toWrite->buf + toWrite->size, buffer, x);
  toWrite->size += x;
  extras = bytes - x;

  if(toWrite->size == toWrite->maxsize) {
    if (write(fd, toWrite->buf, toWrite->size) != (ssize_t)toWrite->size) {
      perror("james disk write error:");
      return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
    }
    toWrite->size = 0;
    if (extras > 0) {
      assert (extras < toWrite->maxsize);
      memcpy(toWrite->buf, buffer+x, extras);
      toWrite->size += extras;
    }
  }
    
  return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

void AlsaTPort::metadata_callback(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
  FLAC__byte b;
  FILE *f;
  AlsaTPort *obj = (AlsaTPort*)client_data;
  int fd;
  const FLAC__uint64 samples = metadata->data.stream_info.total_samples;
  const unsigned min_framesize = metadata->data.stream_info.min_framesize;
  const unsigned max_framesize = metadata->data.stream_info.max_framesize;

  assert(metadata->type == FLAC__METADATA_TYPE_STREAMINFO);

  /*
   * we get called by the encoder when the encoding process has
   * finished so that we can update the STREAMINFO and SEEKTABLE
   * blocks.
   */

  (void)encoder; /* silence compiler warning about unused parameter */

  // close and reopen file in non-direct mode
  close(obj->fd);
  fd = open64(obj->filename, O_WRONLY | O_APPEND);
  // write out any data still buffered
  write(fd, obj->wbuffer->buf, obj->wbuffer->size);
  close(fd);
	
  if(0 == (f = fopen(obj->filename, "r+b")))
    return;

  /* all this is based on intimate knowledge of the stream header
   * layout, but a change to the header format that would break this
   * would also break all streams encoded in the previous format.
   */

  if(-1 == fseek(f, 26, SEEK_SET)) goto samples_;
  fwrite(metadata->data.stream_info.md5sum, 1, 16, f);

 samples_:
  if(-1 == fseek(f, 21, SEEK_SET)) goto framesize_;
  if(fread(&b, 1, 1, f) != 1) goto framesize_;
  if(-1 == fseek(f, 21, SEEK_SET)) goto framesize_;
  b = (b & 0xf0) | (FLAC__byte)((samples >> 32) & 0x0F);
  if(fwrite(&b, 1, 1, f) != 1) goto framesize_;
  b = (FLAC__byte)((samples >> 24) & 0xFF);
  if(fwrite(&b, 1, 1, f) != 1) goto framesize_;
  b = (FLAC__byte)((samples >> 16) & 0xFF);
  if(fwrite(&b, 1, 1, f) != 1) goto framesize_;
  b = (FLAC__byte)((samples >> 8) & 0xFF);
  if(fwrite(&b, 1, 1, f) != 1) goto framesize_;
  b = (FLAC__byte)(samples & 0xFF);
  if(fwrite(&b, 1, 1, f) != 1) goto framesize_;

 framesize_:
  if(-1 == fseek(f, 12, SEEK_SET)) goto end_;
  b = (FLAC__byte)((min_framesize >> 16) & 0xFF);
  if(fwrite(&b, 1, 1, f) != 1) goto end_;
  b = (FLAC__byte)((min_framesize >> 8) & 0xFF);
  if(fwrite(&b, 1, 1, f) != 1) goto end_;
  b = (FLAC__byte)(min_framesize & 0xFF);
  if(fwrite(&b, 1, 1, f) != 1) goto end_;
  b = (FLAC__byte)((max_framesize >> 16) & 0xFF);
  if(fwrite(&b, 1, 1, f) != 1) goto end_;
  b = (FLAC__byte)((max_framesize >> 8) & 0xFF);
  if(fwrite(&b, 1, 1, f) != 1) goto end_;
  b = (FLAC__byte)(max_framesize & 0xFF);
  if(fwrite(&b, 1, 1, f) != 1) goto end_;

 end_:
  fflush(f);
  fclose(f);
  return;
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

