#include "gtest/gtest.h"

#include <jack/ringbuffer.h>
#include "FLAC++/metadata.h"

#include "spool.h"
#include "meter.h"

extern "C" {
#include <openssl/md5.h>
}


class MeterTest : public ::testing::Test {
public:
  jack_ringbuffer_t *ring;
  jack_ringbuffer_data_t writevec[2];
  pthread_mutex_t meter_lock;
  pthread_cond_t  data_ready;
  char tmpl[80];
  char *f;
  MD5_CTX ctx;

  virtual void SetUp() {
    ring = jack_ringbuffer_create(48000 * 4 * 2);
    pthread_mutex_init(&meter_lock, NULL);
    pthread_cond_init(&data_ready, NULL);
    strncpy(tmpl, "/tmp/SpoolTest.XXXXXX", sizeof(tmpl));
    f = mktemp(tmpl);
    MD5_Init(&ctx);
  }

  virtual void TearDown() {
    jack_ringbuffer_free(ring);
    pthread_mutex_destroy(&meter_lock);
    pthread_cond_destroy(&data_ready);
  }

  virtual void pushBlock(int start, int count, bool hash = true) {
    jack_ringbuffer_get_write_vector(ring, writevec);
    int n = std::min((int)writevec[0].len, count * 4);
    int x;
    x = start;
    for (int i = 0; (i*4)<n;i++){
      memcpy(writevec[0].buf+(i*4), &x, 4);
      if(hash) {
        MD5_Update(&ctx, (unsigned char *)&x, 3); // fails on big endian arch
      }
      ++x;
    }
    jack_ringbuffer_write_advance(ring, n);
  
    n = count * 4 - n;
    if (n > 0) {
      for (int z = 0;z*4<n;z++){
        memcpy(writevec[1].buf+(z*4), &x, 4);
        if(hash) {
          MD5_Update(&ctx, (unsigned char *)&x, 3); // fails on big endian arch
        }
        ++x;
      }
      jack_ringbuffer_write_advance(ring, n);
    }

    pthread_cond_signal (&data_ready);
    pthread_mutex_unlock (&meter_lock);

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

};

TEST_F(MeterTest, PumpOne) {
  const int count = 32;
  Spool s(5, count*4, 24, 48000, 2, false, false);
  Meter m((unsigned)2, (unsigned)48000, ring, &s, &meter_lock, &data_ready);

  pushBlock(0, count);
  m.tick();
  s.start(f);
  s.tick();
  s.finish();
  s.finishFLAC();
  verify();
}

TEST_F(MeterTest, ExtraTicks) {
  const int count = 32;
  Spool s(5, count*4, 24, 48000, 2, false, false);
  Meter m((unsigned)2, (unsigned)48000, ring, &s, &meter_lock, &data_ready);

  pushBlock(0, count);
  m.tick();
  m.tick();
  m.tick();
  s.start(f);
  m.tick();
  m.tick();
  m.tick();
  s.tick();
  s.finish();
  s.finishFLAC();
  verify();
}

TEST_F(MeterTest, PumpFiveFirst) {
  const int count = 32;
  Spool s(5, count*4, 24, 48000, 2, false, false);
  Meter m((unsigned)2, (unsigned)48000, ring, &s, &meter_lock, &data_ready);

  ASSERT_EQ(0, m.getmaxn(0));
  ASSERT_EQ(0, m.getmaxn(1));

  pushBlock(0, count);
  pushBlock(count*1, count);
  pushBlock(count*2, count);
  pushBlock(count*3, count);
  pushBlock(count*4, count);
  for(int i=0;i<5;i++)
    m.tick();
  s.start(f);
  int i;
  do {
    i = s.tick();
  } while(i > 0);

  s.finish();
  s.finishFLAC();
  verify();
  ASSERT_EQ(5*32 - 2, m.getmaxn(0));
  ASSERT_EQ(5*32 - 1, m.getmaxn(1));

  m.resetmax();

  ASSERT_EQ(0, m.getmaxn(0));
  ASSERT_EQ(0, m.getmaxn(1));

}

TEST_F(MeterTest, PumpFiveAfter) {
  const int count = 32;
  Spool s(5, count*4, 24, 48000, 2, false, false);
  Meter m((unsigned)2, (unsigned)48000, ring, &s, &meter_lock, &data_ready);

  s.start(f);
  pushBlock(0, count);
  pushBlock(count*1, count);
  pushBlock(count*2, count);
  pushBlock(count*3, count);
  pushBlock(count*4, count);
  for(int i=0;i<5;i++) {
    m.tick();
    s.tick();
  }
  s.finish();
  s.finishFLAC();
  verify();
}

TEST_F(MeterTest, PumpFiveInterleaved) {
  const int count = 32;
  Spool s(5, count*4, 24, 48000, 2, false, false);
  Meter m((unsigned)2, (unsigned)48000, ring, &s, &meter_lock, &data_ready);

  s.start(f);
  pushBlock(0, count);
  m.tick();
  s.tick();
  pushBlock(count*1, count);
  m.tick();
  s.tick();
  pushBlock(count*2, count);
  m.tick();
  s.tick();
  pushBlock(count*3, count);
  m.tick();
  s.tick();
  pushBlock(count*4, count);
  m.tick();
  s.tick();
  s.finish();
  s.finishFLAC();
  verify();
}

TEST_F(MeterTest, LoseOne) {
  const int count = 32;
  Spool s(5, count*4, 24, 48000, 2, false, false);
  Meter m((unsigned)2, (unsigned)48000, ring, &s, &meter_lock, &data_ready);

  pushBlock(0, count, false);
  m.tick();
  for(int i=1;i<=5;i++){
    pushBlock(count*i, count);
    m.tick();
  }
  s.start(f);
  int i;
  do {
    i = s.tick();
  } while(i > 0);
  s.finish();
  s.finishFLAC();
  verify();
}

TEST_F(MeterTest, LoseFive) {
  const int count = 32;
  Spool s(5, count*4, 24, 48000, 2, false, false);
  Meter m((unsigned)2, (unsigned)48000, ring, &s, &meter_lock, &data_ready);

  for(int i=0;i<5;i++){
    pushBlock(count*i, count, false);
    m.tick();
  }
  for(int i=5;i<10;i++){
    pushBlock(count*i, count);
    m.tick();
  }
  s.start(f);
  int i;
  do {
    i = s.tick();
  } while(i > 0);
  s.finish();
  s.finishFLAC();
  verify();

  ASSERT_EQ(318, m.getmaxn(0));
  ASSERT_EQ(319, m.getmaxn(1));

  m.resetmax();

  ASSERT_EQ(0, m.getmaxn(0));
  ASSERT_EQ(0, m.getmaxn(1));
  
}

TEST_F(MeterTest, TLoseFive) {
  const int count = 32;
  Spool s(5, count*4, 24, 48000, 2, true, false);
  Meter m((unsigned)2, (unsigned)48000, ring, &s, &meter_lock, &data_ready);

  m.start();

  for(int i=0;i<5;i++){
    pushBlock(count*i, count, false);
  }
  
  for(int i=5;i<10;i++){
    pushBlock(count*i, count);
  }
  
  usleep(10000);
  s.start(f);
  s.finish();
  s.wait();
  verify();

  ASSERT_EQ(318, m.getmaxn(0));
  ASSERT_EQ(319, m.getmaxn(1));

  m.resetmax();

  ASSERT_EQ(0, m.getmaxn(0));
  ASSERT_EQ(0, m.getmaxn(1));
  
}
