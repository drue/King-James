#include "gtest/gtest.h"

#include <jack/ringbuffer.h>
#include "FLAC++/metadata.h"

#include "spool.h"
#include "meter.h"

class MeterTest : public ::testing::Test {
public:
  jack_ringbuffer_t *ring;
  jack_ringbuffer_data_t writevec[2];
  pthread_mutex_t meter_lock;
  pthread_cond_t  data_ready;
  char tmpl[80];
  char *f;

  virtual void SetUp() {
    ring = jack_ringbuffer_create(48000 * 4 * 2);
    pthread_mutex_init(&meter_lock, NULL);
    pthread_cond_init(&data_ready, NULL);
    strncpy(tmpl, "/tmp/SpoolTest.XXXXXX", sizeof(tmpl));
    f = mktemp(tmpl);
  }

  virtual void TearDown() {
    jack_ringbuffer_free(ring);
    pthread_mutex_destroy(&meter_lock);
    pthread_cond_destroy(&data_ready);
  }

  virtual void pushBlock(int start, int count) {
    jack_ringbuffer_get_write_vector(ring, writevec);
    int n = std::min((int)writevec[0].len, count * 4);
    for (int i = start; i*4<n+start*4;i++){
      memcpy(writevec[0].buf+(i*4), &i, 4);
    }
    jack_ringbuffer_write_advance(ring, n);
  
    n = count * 4 - n;
    if (n > 0) {
      for (int i = 0; i*4<n;i++){
        memcpy(writevec[1].buf+(i*4), &i, 4);
      }
      jack_ringbuffer_write_advance(ring, n);
    }

    pthread_cond_signal (&data_ready);
    pthread_mutex_unlock (&meter_lock);

  }

  virtual void verifyMD5(const unsigned char *correct) {
    FLAC::Metadata::StreamInfo si;
    FLAC::Metadata::get_streaminfo(f, si);
    const unsigned char *sum = si.get_md5sum();
    for(int i=0;i<16;i++) {
      ASSERT_EQ(correct[i], sum[i]);
    }    
  }
};

TEST_F(MeterTest, PumpOne) {
  const int count = 32;
  Spool s(5, count*4, 24, 48000, 2);
  Meter m((unsigned)2, (unsigned)48000, ring, &s, &meter_lock, &data_ready);

  pushBlock(0, count);
  m.tick();
  s.start(f);
  s.finish();
  s.wait();
  const unsigned char *correct = (const unsigned char *)"\x7f\x5d\xb3\x3c\x5e\xc6\x27\x3c\xa2\x13\x55\x5b\xe8\x60\x00\x39";
  verifyMD5(correct);
}
