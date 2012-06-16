#include <FLAC/all.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

#include <zmq.hpp>

#include "spool.h"
#include "memq.h"

Spool::Spool(unsigned int prerollSize, unsigned int bufSize, unsigned int bps, unsigned int sr, unsigned int c)
  :  ctx(1), socket(ctx, ZMQ_PUB) {
  Q = new MemQ(prerollSize, bufSize);
  bits_per_sample = bps;
  sample_rate = sr;
  channels = c;
  finished = false;
  started = false;
  oFrames = 0;
  aFrames = 0; 
  lastProgress = 0;
  filename = NULL;
  pthread_mutex_init(&frameLock, NULL);
  pthread_mutex_init(&finishLock, NULL);
  pthread_cond_init(&finishCond, NULL);
  socket.connect("ipc:///tmp/progressIn.ipc");
}

Spool::~Spool() {
  delete(Q);
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

QItem *Spool::getEmpty() {
  return Q->getEmpty();
}

void Spool::pushItem(QItem *item) {
  unsigned int bufLength;

  Q->putTail(item);

  pthread_mutex_lock(&frameLock);
  aFrames += item->size / channels / 4;
  if(started)
    oFrames += item->size / channels / 4;
  pthread_mutex_unlock(&frameLock);

  if (aFrames - lastProgress > sample_rate / 2) {
    lastProgress = aFrames;
    bufLength = Q->getSize() * item->size / 4 / channels / sample_rate;
    sprintf(progMsg, "{\"t\":%.0f, \"m\":%d, \"b\":%d}", floor(oFrames / sample_rate), 
            started ? 1 : 0, bufLength);
    zmq::message_t msg(progMsg, strlen(progMsg), NULL);
    socket.send(msg);
  }
}

void Spool::start(char *savePath) {
  filename = (char *)malloc(strlen(savePath));
  strcpy(filename, savePath);

  pthread_mutex_lock(&frameLock);
  oFrames += (Q->getSize() * Q->bufSize()) / 4 / channels;
  started = true;
  pthread_mutex_unlock(&frameLock);

  // start write to disk
  pthread_create(&sthread, NULL, (void * (*)(void *))doWrite, this);
}

void Spool::doWrite(void *foo) {
  Spool *obj = (Spool *)foo;
  QItem *i;
  FLAC__StreamEncoderInitStatus initted;
  FLAC__StreamEncoder *encoder;
  FILE *output;
  unsigned int s;
  struct timespec ts;

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
    // get QItem from Q
    clock_gettime(CLOCK_REALTIME, &ts) ;
    ts.tv_nsec += 100000000;
    i = obj->Q->getHead(&ts);
    if (i != NULL) {
      // write QItem->buf to disk
      if (!FLAC__stream_encoder_process_interleaved(encoder, (const FLAC__int32 *)i->buf,
                                                    i->size / obj->channels / 4)){
        printf("FLAC error! state = %d:%s \n", FLAC__stream_encoder_get_state(encoder), 
               FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder)]);
      }
      delete i;
    }
    s = obj->Q->getSize();
  } while (s > 0 || (s == 0 && !obj->finished));

  FLAC__stream_encoder_finish(encoder);
  fsync(fileno(output));
  FLAC__stream_encoder_delete(encoder);

  pthread_cond_broadcast(&obj->finishCond);

  delete obj;
}
