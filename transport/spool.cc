#include <deque>
#include <iostream>

#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <zmq.hpp>
#include <FLAC/all.h>

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

Spool::Spool(unsigned int prerollSize, unsigned int bufSize, unsigned int bps, unsigned int sr, unsigned int c, bool spawn, bool progress) 
{
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
  send_progress = progress;
  filename = NULL;
  ready=false;
  done=false;

  should_spawn = spawn;
  if (send_progress) {
    ctx = new zmq::context_t(1);
    socket = new zmq::socket_t(*ctx, ZMQ_PUB);
    socket->connect("ipc:///tmp/progressIn.ipc");
  }
}

Spool::~Spool() {
  if(should_spawn) {
    thread->join();
    delete(thread);
  }

  
  if(filename != NULL)
    free(filename);
  if (send_progress ) {
    delete(socket);
    delete(ctx);
  }
}

void Spool::finish() {
  boost::mutex::scoped_lock lock(qLock);
  finished = true;
  dataCond.notify_all();
}

void Spool::wait() {
  boost::mutex::scoped_lock lock(qLock);
  while(!done)
    finishCond.wait(qLock);
}

void Spool::waitReady() {
  boost::mutex::scoped_lock lock(qLock);
  while(!ready)
    finishCond.wait(qLock);
}

buffer& Spool::getEmpty() {
  buffer *buf = new buffer(bufferSize);
  return *buf;
}

void Spool::pushItem(buffer& buf) {
  unsigned int bufLength;

  qLock.lock();
  Q.push_back(buf);
  if(started == false) {
    while (Q.size() > maxQSize) {
      Q.pop_front();
    }
  }
  dataCond.notify_all();
  qLock.unlock();

  aFrames += buf.size / channels / 4;
  if(started)
    oFrames += buf.size / channels / 4;


  if (send_progress && aFrames - lastProgress > sample_rate / channels / 2) {
    lastProgress = aFrames;
    bufLength = Q.size() * buf.size / 4 / channels / sample_rate;
    sprintf(progMsg, "{\"t\":%.0f, \"m\":%d, \"b\":%d}", floor(oFrames / sample_rate), 
            started ? 1 : 0, bufLength);
    zmq::message_t msg(progMsg, strlen(progMsg), NULL);
    socket->send(msg);
  }
}

void Spool::start(char *savePath) {
  filename = (char *)malloc(strlen(savePath));
  strcpy(filename, savePath);

  boost::mutex::scoped_lock lock(qLock);
  oFrames += (Q.size() * bufferSize) / 4 / channels;
  started = true;

  // start write to disk
  if (should_spawn)
    thread = new boost::thread(&doWrite, this);
}

int Spool::tick() {
  qLock.lock();
  int s = Q.size();
  if (s > 0) {
    buffer& i = Q.front();
    //printf("Encoding %d - %d\n", i.buf[0], i.buf[i.size / 4 - 1]);
    // write QItem->buf to disk
    qLock.unlock();
    if (!FLAC__stream_encoder_process_interleaved(encoder, (const FLAC__int32 *)i.buf,
                                                  i.size / channels / 4)){
      printf("FLAC error! state = %d:%s \n", FLAC__stream_encoder_get_state(encoder), 
             FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder)]);
    }
    qLock.lock();
    Q.pop_front();
    qLock.unlock();
    return s-1;
  }
  else {
    qLock.unlock();
    return 0;
  }
}

void Spool::initFLAC() {
  FLAC__StreamEncoderInitStatus initted;
  output = fopen64(filename, "w+b");

  // only cache beginning of this huge file we're about to write out and not reuse
  posix_fadvise(fileno(output), 8192, 0, POSIX_FADV_NOREUSE);

  encoder = FLAC__stream_encoder_new();
  FLAC__stream_encoder_set_channels(encoder, channels);
  FLAC__stream_encoder_set_bits_per_sample(encoder, bits_per_sample);
  FLAC__stream_encoder_set_sample_rate(encoder, sample_rate);
  //FLAC__stream_encoder_set_compression_level(encoder, 8);
  initted = FLAC__stream_encoder_init_FILE(encoder, output, NULL, NULL);
  if (initted != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
    {
      printf("Couldn't initialize FLAC encoder.\n");
      exit(-1);
    }
  
  chmod(filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}
void Spool::doWrite(void *foo) {
  Spool *obj = (Spool *)foo;
  unsigned int s;
  bool exiting = false;

  obj->qLock.lock();
  if(!obj->ready) {
    obj->initFLAC();
    obj->ready=true;
    obj->finishCond.notify_all();
  }
  obj->qLock.unlock();

  do {
    obj->tick();
     
    obj->qLock.lock();
    s = obj->Q.size();
    if(obj->finished && s == 0)
      exiting = true;
    else if (s == 0) {
      obj->dataCond.wait(obj->qLock);
    }
    obj->qLock.unlock();
  } while (!exiting);

  obj->qLock.lock();
  obj->finishFLAC(); 
  obj->done = true;
  obj->finishCond.notify_all();
  obj->qLock.unlock();
}

void Spool::finishFLAC() {
  FLAC__stream_encoder_finish(encoder);
  fsync(fileno(output));
  FLAC__stream_encoder_delete(encoder);
}
