#include <deque>
#include <iostream>

#include <FLAC/all.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

#include <zmq.hpp>

#include "spool.h"

buffer::buffer(unsigned int bsize) {
  // std::cout  <<  "Buffer  :  ctor\r\n";
  size = bsize;
  buf = (int *)malloc(bsize);
}

buffer::~buffer() {
  // std::cout  <<  "Buffer  :  dtor\r\n";
  free(buf);
}

Spool::Spool(unsigned int prerollSize, unsigned int bufSize, unsigned int bps, unsigned int sr, unsigned int c)
  :  ctx(1), socket(ctx, ZMQ_PUB) {
  bits_per_sample = bps;
  sample_rate = sr;
  channels = c;
  bufferSize = bufSize;
  maxQSize = prerollSize;
  finished = false;
  started = false;
  oFrames = 0;
  aFrames = 0; 
  lastProgress = 0;
  filename = NULL;
  pthread_mutex_init(&qLock, NULL);
  pthread_mutex_init(&finishLock, NULL);
  pthread_cond_init(&finishCond, NULL);
  socket.connect("ipc:///tmp/progressIn.ipc");
}

Spool::~Spool() {
  if(filename != NULL)
    free(filename);
}

void Spool::finish() {
  pthread_mutex_lock(&finishLock);
  finished = true;
  pthread_mutex_unlock(&finishLock);
}

void Spool::wait() {
  pthread_mutex_lock(&finishLock);
  pthread_cond_wait(&finishCond, &finishLock);
  pthread_mutex_unlock(&finishLock);
}

const buffer& Spool::getEmpty() {
  buffer *buf = new buffer(bufferSize);
  return *buf;
}

void Spool::pushItem(const buffer& buf) {
  unsigned int bufLength;

  pthread_mutex_lock(&qLock);
  Q.push_back(buf);
  if(!started) {
    while (Q.size() > maxQSize) {
      Q.pop_front();
    }
  }


  aFrames += buf.size / channels / 4;
  if(started)
    oFrames += buf.size / channels / 4;

  if (aFrames - lastProgress > sample_rate / 2) {
    lastProgress = aFrames;
    bufLength = Q.size() * buf.size / 4 / channels / sample_rate;
    sprintf(progMsg, "{\"t\":%.0f, \"m\":%d, \"b\":%d}", floor(oFrames / sample_rate), 
            started ? 1 : 0, bufLength);
    zmq::message_t msg(progMsg, strlen(progMsg), NULL);
    socket.send(msg);
  }
  pthread_mutex_unlock(&qLock);
}

void Spool::start(char *savePath) {
  filename = (char *)malloc(strlen(savePath));
  strcpy(filename, savePath);

  pthread_mutex_lock(&qLock);
  oFrames += (Q.size() * bufferSize) / 4 / channels;
  started = true;
  pthread_mutex_unlock(&qLock);

  // start write to disk
  pthread_create(&sthread, NULL, (void * (*)(void *))doWrite, this);
}

void Spool::doWrite(void *foo) {
  Spool *obj = (Spool *)foo;
  FLAC__StreamEncoderInitStatus initted;
  FLAC__StreamEncoder *encoder;
  FILE *output;
  unsigned int s;

  output = fopen64(obj->filename, "w+b");

  // only cache beginning of this huge file we're about to write out and not reuse
  posix_fadvise(fileno(output), 8192, 0, POSIX_FADV_NOREUSE);

  encoder = FLAC__stream_encoder_new();
  FLAC__stream_encoder_set_channels(encoder, obj->channels);
  FLAC__stream_encoder_set_bits_per_sample(encoder, obj->bits_per_sample);
  FLAC__stream_encoder_set_sample_rate(encoder, obj->sample_rate);
  initted = FLAC__stream_encoder_init_FILE(encoder, output, NULL, NULL);
  if (initted != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
    {
      printf("Couldn't initialize FLAC encoder.\n");
      exit(-1);
    }

  do {
    s = obj->Q.size();
    if (s > 0) {
      buffer& i = obj->Q.front();
      // write QItem->buf to disk
      if (!FLAC__stream_encoder_process_interleaved(encoder, (const FLAC__int32 *)i.buf,
                                                    i.size / obj->channels / 4)){
        printf("FLAC error! state = %d:%s \n", FLAC__stream_encoder_get_state(encoder), 
               FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder)]);
      }
      pthread_mutex_lock(&obj->qLock);
      obj->Q.pop_front();
      pthread_mutex_unlock(&obj->qLock);
    }
    else {
      usleep(10000);
    }
  } while (s > 0 || (s == 0 && !obj->finished));

  FLAC__stream_encoder_finish(encoder);
  fsync(fileno(output));
  FLAC__stream_encoder_delete(encoder);

  pthread_cond_broadcast(&obj->finishCond);
}
