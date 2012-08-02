#include <stdlib.h>
#include <arpa/inet.h>

#include "gtest/gtest.h"
#include "FLAC++/metadata.h"
#include <openssl/md5.h>

#include "spool.h"

/*
*************
** FLAC md5sum is computed from byte aligned, little endian data
** 3 bytes per sample for 24 bits, 2 bytes for 16 bits
** however, data is fed to FLAC with 4 byte unsigned ints
*************
*/

/*
** test data consists of monotonically increasing integers from zero, so out-of-order or missing samples can be 
** easily detected
*/

class SpoolTest : public ::testing::Test {
public:
  char tmpl[80];
  char *f;
  MD5_CTX ctx;
  Spool *s;
  unsigned int width;

  virtual void SetUp() {
    width = 3;
    strncpy(tmpl, "/tmp/SpoolTest.XXXXXX", sizeof(tmpl));
    f = mktemp(tmpl);
    MD5_Init(&ctx);
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

  virtual int pushBlock(int start, bool hash=true) {
    buffer& item = s->getEmpty();
    unsigned int x;
    for ( x=0; x*4 < item.size; x++) {
      int z = start + x;
      item.buf[x] = z;
      if(hash) {
        MD5_Update(&ctx, (unsigned char *)&z, width); // only works on little endian arch
      }
    }
    s->pushItem(item);
    return start + x;
  }
};

TEST_F(SpoolTest, DeallocUnused) {
  Spool s(128*5, 128, 24, 44100, 2, false);
}

TEST_F(SpoolTest, Null) {

  s = new Spool(5, 128, 24, 48000, 2, false);
  s->start(f);
  s->initFLAC();
  s->finish();
  s->finishFLAC();
  verify();
  delete(s);
}

TEST_F(SpoolTest, OneItem) {
  s = new Spool(5, 128, 24, 48000, 2, false);
  
  s->start(f);
  s->initFLAC();
  pushBlock(0);
  s->tick();
  s->finish();
  s->finishFLAC();
       
  verify();
  delete(s);
}

TEST_F(SpoolTest, FiveItems) {
  s = new Spool(5, 128, 24, 48000, 2, false);
  
  s->start(f);
  s->initFLAC();

  int n = 0;
  for(int x=0;x<5;x++) {
    n = pushBlock(n);
    s->tick();
  }
  s->finish();
  s->finishFLAC();
       
  verify();
}

TEST_F(SpoolTest, SpoolUp5) {
  s = new Spool(5, 128, 24, 48000, 2, false);
  
  
  int n = 0;
  for(int x=0;x<5;x++) {
    n = pushBlock(n);
  }

  s->start(f);
  s->initFLAC();
  unsigned int x;
  do {
    x = s->tick();
  } while (x > 0);

  s->finish();
  s->finishFLAC();
       
  verify();
  delete(s);
}


TEST_F(SpoolTest, HalfAndHalf) {
  s = new Spool(5, 128, 24, 48000, 2, false);
  
  // fill up the buffer reroll buffer, start, then send another five below
  int n = 0;
  for(int x=0;x<5;x++) {
    n = pushBlock(n);
  }

  s->start(f);
  s->initFLAC();

  unsigned int x;
  do {
    x = s->tick();
  } while (x > 0);

  for(int x=5;x<10;x++) {
    n = pushBlock(n);
    s->tick();
  }


  s->finish();
  s->finishFLAC();
       
  verify();
  delete(s);
}

TEST_F(SpoolTest, HalfAndHalf16) {
  width = 2;
  s = new Spool(5, 128, 16, 48000, 2, false);
  
  int n = 0;
  for(int x=0;x<5;x++) {
    n = pushBlock(n);
  }

  s->start(f);
  s->initFLAC();
  unsigned int x;
  do {
    x = s->tick();
  } while (x > 0);

  for(int x=0;x<5;x++) {
    n = pushBlock(n);
    s->tick();
  }


  s->finish();
  s->finishFLAC();
       
  verify();
  delete(s);
}

TEST_F(SpoolTest, LoseOne) {
  s = new Spool(5, 128, 24, 48000, 2, false);
  
  // fill up the buffer reroll buffer, start, then send another five below
  int n = 0;

  n = pushBlock(n, false);
  for(int x=0;x<5;x++) {
    n = pushBlock(n);
  }

  s->start(f);
  s->initFLAC();
  unsigned int x;
  do {
    x = s->tick();
  } while (x > 0);

  for(int x=0;x<5;x++) {
    n = pushBlock(n);
    s->tick();
  }


  s->finish();
  s->finishFLAC();
       
  verify();
  delete(s);
}

TEST_F(SpoolTest, LoseFive) {
  s = new Spool(5, 128, 24, 48000, 2, false);
  
  // fill up the buffer reroll buffer, start, then send another five below
  int n = 0;
  for(int x=0;x<5;x++) {
    n = pushBlock(n, false);
  }

  for(int x=0;x<5;x++) {
    n = pushBlock(n);
  }

  s->start(f);
  s->initFLAC();
  unsigned int x;
  do {
    x = s->tick();
  } while (x > 0);

  for(int x=0;x<5;x++) {
    n = pushBlock(n);
    s->tick();
  }


  s->finish();
  s->finishFLAC();
       
  verify();
  delete(s);
}


TEST_F(SpoolTest, TLoseFive) {
  s = new Spool(5, 128, 24, 48000, 2, true, false);
  
  // fill up the buffer reroll buffer, start, then send another five below
  int n = 0;
  for(int x=0;x<5;x++) {
    n = pushBlock(n, false);
  }

  for(int x=0;x<5;x++) {
    n = pushBlock(n);
  }

  s->start(f);
  s->waitReady();

  for(int x=0;x<5;x++) {
    n = pushBlock(n);
  }

  s->finish();
  s->wait();
       
  verify();
  delete(s);
}


TEST_F(SpoolTest, TPump) {
  s = new Spool(5, 128, 24, 48000, 2, true, false);

  int n = 0;
  // fill up the buffer reroll buffer, start, then send another five below
  for(int x=0;x<5;x++) {
    n = pushBlock(n);
  }

  s->start(f);
  s->waitReady();

  for(int x=0;x<500;x++) {
    n = pushBlock(n);
  }
  s->finish();
  s->wait();
       
  verify();
  delete(s);
}

TEST_F(SpoolTest, TTenBig) {
  s = new Spool(5, 8000, 24, 48000, 2, true, false);

  int n = 0;
  // fill up the buffer reroll buffer, start, then send another five below
  for(int x=0;x<5;x++) {
    n = pushBlock(n);
  }

  s->start(f);
  s->waitReady();

  for(int x=0;x<5;x++) {
    n = pushBlock(n);
  }
  s->finish();
  s->wait();
       
  verify();
  delete(s);
}

TEST_F(SpoolTest, TFiftyBig) {
  s = new Spool(5, 8000, 24, 48000, 2, true, false);

  int n = 0;
  // fill up the buffer reroll buffer, start, then send another five below
  for(int x=0;x<5;x++) {
    n = pushBlock(n);
  }

  s->start(f);
  s->waitReady();

  for(int x=0;x<50;x++) {
    n = pushBlock(n);
  }
  s->finish();
  s->wait();
  verify();
  delete(s);
}

TEST_F(SpoolTest, TBiggie) {
  s = new Spool(5, 3200*8, 24, 48000, 2, true, false);

  int n = 0;

  for(int x=0;x<5;x++) {
    n = pushBlock(n);
  }

  s->start(f);
  s->waitReady();

  for(int x=0;x<500;x++) {
    n = pushBlock(n);
  }

  s->finish();
  s->wait();
  verify();
  delete(s);
}
