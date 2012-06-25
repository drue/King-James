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
};


TEST_F(TransportTest, Simple) {
  AlsaTPort port("0", 24, 48000, false);
  tt = this;
  
  port.tick(&reader);
  port.meter->tick();

  port.startRecording(f);

  port.spool->finish();
  port.spool->wait();
  verify();
}

TEST_F(TransportTest, NowNLater) {
  AlsaTPort port("0", 24, 48000, false);
  tt = this;

  port.tick(&reader);
  port.meter->tick();

  port.startRecording(f);

  port.tick(&reader);
  port.meter->tick();

  port.spool->finish();
  port.spool->wait();
  verify();
}

TEST_F(TransportTest, Ten) {
  AlsaTPort port("0", 24, 48000, false);
  tt = this;

  for(int i=0;i<5;i++) {
    port.tick(&reader);
    port.meter->tick();
  }

  port.startRecording(f);

  for(int i=0;i<5;i++) {
    port.tick(&reader);
    port.meter->tick();
  }

  port.spool->finish();
  port.spool->wait();
  verify();
}
