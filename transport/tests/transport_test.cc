#include "gtest/gtest.h"

#include <jack/ringbuffer.h>
#include "FLAC++/metadata.h"

#include "Transport.h"

extern "C" {
#include <openssl/md5.h>
}
class TransportTest;

static TransportTest *tt;


class TransportTest : public ::testing::Test {
public:
  char tmpl[80];
  char *f;
  MD5_CTX ctx;
  int counter;
  int channels;

  virtual void SetUp() {
    strncpy(tmpl, "/tmp/SpoolTest.XXXXXX", sizeof(tmpl));
    f = mktemp(tmpl);
    MD5_Init(&ctx);
    counter = 0;
    channels = 2;
    tt = this;
  }

  virtual void TearDown() {
  }

  virtual void verify() {
    FLAC::Metadata::StreamInfo si;
    FLAC::Metadata::get_streaminfo(f, si);
    const unsigned char *sum = si.get_md5sum();

    unsigned char  ohash[16];
    MD5_Final(ohash, &ctx);

    for(int i=0;i<16;i++) {
      ASSERT_EQ(ohash[i], sum[i]);
    }    
  }

  static snd_pcm_sframes_t reader(snd_pcm_t *handle, void *buf, snd_pcm_uframes_t frames) {
    for (unsigned int i=0;i<frames*tt->channels;i++) {
      ((int *)buf)[i] = tt->counter;
      MD5_Update(&tt->ctx, &tt->counter, 3); // only works on little endian
      tt->counter++;
    }
    return frames;
  }

  static snd_pcm_sframes_t rand_reader(snd_pcm_t *handle, void *buf, snd_pcm_uframes_t frames) {
    unsigned nframes = std::max(1, (int)(frames * (random() * 1.0 / RAND_MAX)));

    for (unsigned int i=0;i<nframes*tt->channels;i++) {
      ((int *)buf)[i] = tt->counter;
      MD5_Update(&tt->ctx, &tt->counter, 3); // only works on little endian
      tt->counter++;
    }
    return nframes;
  }
};

TEST_F(TransportTest, Simple) {
  unsigned int SR = 48000;
  AlsaTPort port("0", 24, SR, SR/32, SR/10, false);
  
  port.tick(&reader);
  port.meter->tick();

  port.startRecording(f);
  port.spool->initFLAC();
  port.spool->tick();
  port.spool->finish();
  port.spool->finishFLAC();
  verify();
}

TEST_F(TransportTest, NowNLater) {
  unsigned int SR = 48000;
  AlsaTPort port("0", 24, SR, SR/32, SR/10, false);

  port.tick(&reader);
  port.meter->tick();

  port.startRecording(f);
  port.spool->initFLAC();
  port.spool->tick();

  port.tick(&reader);
  port.meter->tick();
  port.spool->tick();

  port.spool->finish();
  port.spool->finishFLAC();
  verify();
}

TEST_F(TransportTest, Ten) {
  unsigned int SR = 48000;
  AlsaTPort port("0", 24, SR, SR/32, SR/10, false);

  for(int i=0;i<5;i++) {
    port.tick(&reader);
    port.meter->tick();
  }

  port.startRecording(f);
  port.spool->initFLAC();
  int x;
  do {
    x = port.spool->tick();
  } while (x > 0);

  for(int i=0;i<5;i++) {
    port.tick(&reader);
    port.meter->tick();
    port.spool->tick();
  }

  port.spool->finish();
  port.spool->finishFLAC();
  verify();
}

TEST_F(TransportTest, 1K) {
  unsigned int SR = 48000;
  AlsaTPort port("0", 24, SR, SR/32, SR/10, false);

  for(int i=0;i<5;i++) {
    port.tick(&reader);
    port.meter->tick();
  }

  port.startRecording(f);
  port.spool->initFLAC();

  int x;
  do {
    x = port.spool->tick();
  } while (x > 0);


  for(int i=0;i<500;i++) {
    port.tick(&reader);
    port.meter->tick();
    port.spool->tick();
  }

  port.spool->finish();
  port.spool->finishFLAC();
  verify();
}

TEST_F(TransportTest, T1K) {
  unsigned int SR = 48000;
  int count = 3200;
  AlsaTPort port("0", 24, SR, 10, 0, false);

  port.meter->start();
  port.meter->waitReady();
  port.spool->should_spawn = true;


  for(int i=0;i<5;i++) {
    port.tick(&reader);
  }

  port.startRecording(f);

  for(int i=0;i<500;i++) {
    port.tick(&reader);
    usleep(count * (1/(SR * 10.0))*1000000);
  }

  port.meter->finish();
  port.meter->wait();
  port.spool->finish();
  port.spool->wait();
  verify();
}

TEST_F(TransportTest, T10K) {
  unsigned int SR = 48000;
  int count = SR/10*2;
  AlsaTPort port("0", 24, SR, 10, 0, false);

  port.meter->start();
  port.meter->waitReady();
  port.spool->should_spawn = true;


  for(int i=0;i<5;i++) {
    port.tick(&reader);
  }

  port.startRecording(f);

  for(int i=0;i<500;i++) {
    port.tick(&reader);
    usleep(count * (1/(SR * 10.0))*1000000);
  }

  port.meter->finish();
  port.meter->wait();
  port.spool->finish();
  port.spool->wait();
  verify();
}

TEST_F(TransportTest, RandT1K) {
  unsigned int SR = 48000;
  int count = 3200;
  AlsaTPort port("0", 24, SR, 10, 0, false);

  port.meter->start();
  port.meter->waitReady();
  port.spool->should_spawn = true;


  for(int i=0;i<5;i++) {
    port.tick(&rand_reader);
  }

  port.startRecording(f);

  for(int i=0;i<500;i++) {
    port.tick(&rand_reader);
    unsigned c = count * (1/(SR * 10.0))*1000000;
    usleep(c);
  }

  port.meter->finish();
  port.meter->wait();
  port.spool->finish();
  port.spool->wait();
  verify();
}

TEST_F(TransportTest, Long) {
  unsigned int SR = 48000;
  int count = 3200;
  AlsaTPort port("0", 24, SR, 10, 0, false);

  port.meter->start();
  port.meter->waitReady();
  port.spool->should_spawn = true;


  for(int i=0;i<5;i++) {
    port.tick(&rand_reader);
  }

  port.startRecording(f);

  for(int i=0;i<50000;i++) {
    port.tick(&rand_reader);
    unsigned c = count * (1/(SR * 10.0))*1000000;
    usleep(c);
  }

  port.meter->finish();
  port.meter->wait();
  port.spool->finish();
  port.spool->wait();
  verify();
}
