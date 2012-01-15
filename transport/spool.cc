#include <FLAC/all.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "spool.h"
#include "memq.h"

Spool::Spool(unsigned int prerollSize, unsigned int bps, unsigned int sr) {
  Q = new MemQ(prerollSize, 0);
  bits_per_sample = bps;
  sample_rate = sr;
  finished = false;
}

Spool::~Spool() {
  delete(Q);
  free(filename);
}

void Spool::finish() {
  finished = true;
}

void Spool::pushItem(QItem *item) {
  Q->putTail(item);
}

void Spool::start(char *savePath) {
  filename = (char *)malloc(strlen(savePath));
  strcpy(filename, savePath);

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

  output = fopen64(obj->filename, "w+");

  // only cache beginning of this huge file we're about to write out and not reuse
  posix_fadvise(fileno(output), 8192, 0, POSIX_FADV_NOREUSE);

  encoder = FLAC__stream_encoder_new();
  FLAC__stream_encoder_set_channels(encoder, 2);
  FLAC__stream_encoder_set_bits_per_sample(encoder, obj->bits_per_sample);
  FLAC__stream_encoder_set_sample_rate(encoder, obj->sample_rate);
  FLAC__stream_encoder_set_do_mid_side_stereo(encoder, 0);
  FLAC__stream_encoder_set_blocksize(encoder, 4608);
  FLAC__stream_encoder_set_max_lpc_order(encoder, 0);
  FLAC__stream_encoder_set_min_residual_partition_order(encoder, 3);
  FLAC__stream_encoder_set_max_residual_partition_order(encoder, 3);

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
      if (!FLAC__stream_encoder_process_interleaved(encoder, (const FLAC__int32 *)i->buf, i->frames)){
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


  delete obj;
}