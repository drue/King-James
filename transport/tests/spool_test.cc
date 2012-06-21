#include <stdlib.h>
#include <arpa/inet.h>

#include "gtest/gtest.h"
#include "FLAC++/metadata.h"
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

  virtual void SetUp() {
    strncpy(tmpl, "/tmp/SpoolTest.XXXXXX", sizeof(tmpl));
    f = mktemp(tmpl);
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

TEST_F(SpoolTest, DeallocUnused) {
  Spool s(128*5, 128, 24, 44100, 2);
}

TEST_F(SpoolTest, Null) {

  Spool s(5, 128, 24, 48000, 2);
  const unsigned char *emptymd5 = (const unsigned char *)"\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42\x7e";

  s.start(f);
  s.finish();
  s.wait();
  verifyMD5(emptymd5);
}

TEST_F(SpoolTest, OneItem) {
  const unsigned char *correct = (const unsigned char *)"\x7f\x5d\xb3\x3c\x5e\xc6\x27\x3c\xa2\x13\x55\x5b\xe8\x60\x00\x39";
  Spool s(5, 128, 24, 48000, 2);
  
  s.start(f);

  buffer& item = s.getEmpty();
  unsigned int i;
  for (i=0; i*4 < item.size; i++) {
    item.buf[i] = i;
  }
  s.pushItem(item);
  s.finish();
  s.wait();
       
  verifyMD5(correct);
}

TEST_F(SpoolTest, FiveItems) {
  const unsigned char *correct = (const unsigned char *)"\xae\x20\x76\x57\x55\x7a\xa6\xa7\x19\xcd\x58\x68\x5d\xfc\x76\xdb"; // ints going from 0 to 5*32
  Spool s(5, 128, 24, 48000, 2);
  
  s.start(f);

  for(int x=0;x<5;x++) {
    buffer& item = s.getEmpty();
    for (unsigned int i=0; i*4 < item.size; i++) {
      item.buf[i] = x*32 + i;
    }
    s.pushItem(item);
  }
  s.finish();
  s.wait();
       
  verifyMD5(correct);
}

TEST_F(SpoolTest, SpoolUp5) {
  const unsigned char *correct = (const unsigned char *)"\xae\x20\x76\x57\x55\x7a\xa6\xa7\x19\xcd\x58\x68\x5d\xfc\x76\xdb"; // ints going from 0 to 5*32
  Spool s(5, 128, 24, 48000, 2);
  

  for(int x=0;x<5;x++) {
    buffer& item = s.getEmpty();
    for (unsigned int i=0; i*4 < item.size; i++) {
      item.buf[i] = x*32 + i;
    }
    s.pushItem(item);
  }

  s.start(f);
  s.finish();
  s.wait();
       
  verifyMD5(correct);
}

TEST_F(SpoolTest, HalfAndHalf) {
  const unsigned char *correct = (const unsigned char *)"\x70\xd3\x26\x79\xb7\x2b\xe8\xcf\x6f\x7d\x61\x50\x4d\xdd\x06\x03"; // ints going from 0 to 10*32
  Spool s(5, 128, 24, 48000, 2);
  
  // fill up the buffer reroll buffer, start, then send another five below
  for(int x=0;x<5;x++) {
    buffer& item = s.getEmpty();
    for (unsigned int i=0; i*4 < item.size; i++) {
      item.buf[i] = x*32 + i;
    }
    s.pushItem(item);
  }

  s.start(f);

  for(int x=5;x<10;x++) {
    buffer& item = s.getEmpty();
    for (unsigned int i=0; i*4 < item.size; i++) {
      item.buf[i] = x*32 + i;
    }
    s.pushItem(item);
  }


  s.finish();
  s.wait();
       
  verifyMD5(correct);
}

TEST_F(SpoolTest, HalfAndHalf16) {
  const unsigned char *correct = (const unsigned char *)"\x79\x25\xec\x2c\xdd\x77\xe0\x56\x72\x45\x1f\x83\xf6\x35\xa0\x4a"; // shorts going from 0 to 10*32
  Spool s(5, 128, 16, 48000, 2);
  

  for(int x=0;x<5;x++) {
    buffer& item = s.getEmpty();
    for (unsigned int i=0; i*4 < item.size; i++) {
      item.buf[i] = x*32 + i;
    }
    s.pushItem(item);
  }

  s.start(f);

  for(int x=5;x<10;x++) {
    buffer& item = s.getEmpty();
    for (unsigned int i=0; i*4 < item.size; i++) {
      item.buf[i] = x*32 + i;
    }
    s.pushItem(item);
  }


  s.finish();
  s.wait();
       
  verifyMD5(correct);
}

TEST_F(SpoolTest, LoseOne) {
  const unsigned char *correct = (const unsigned char *)"\xf0\xa4\x66\x66\x73\x60\xe4\xa4\xfd\x22\x9f\xe0\xba\xf0\x68\xf6"; // ints going from 32 to 11*32
  Spool s(5, 128, 24, 48000, 2);
  
  // fill up the buffer reroll buffer, start, then send another five below
  for(int x=0;x<6;x++) {
    buffer& item = s.getEmpty();
    for (unsigned int i=0; i*4 < item.size; i++) {
      item.buf[i] = x*32 + i;
    }
    s.pushItem(item);
  }

  s.start(f);

  for(int x=6;x<11;x++) {
    buffer& item = s.getEmpty();
    for (unsigned int i=0; i*4 < item.size; i++) {
      item.buf[i] = x*32 + i;
    }
    s.pushItem(item);
  }


  s.finish();
  s.wait();
       
  verifyMD5(correct);
}

TEST_F(SpoolTest, LoseFive) {
  const unsigned char *correct = (const unsigned char *)"\xc5\x72\x99\x44\xbd\x6f\x49\x1f\x24\x79\x6b\xf8\x7f\x8a\x60\xe0"; // ints going from 5*32 to 16*32
  Spool s(5, 128, 24, 48000, 2);
  
  // fill up the buffer reroll buffer, start, then send another five below
  for(int x=0;x<10;x++) {
    buffer& item = s.getEmpty();
    for (unsigned int i=0; i*4 < item.size; i++) {
      item.buf[i] = x*32 + i;
    }
    s.pushItem(item);
  }

  s.start(f);

  for(int x=10;x<16;x++) {
    buffer& item = s.getEmpty();
    for (unsigned int i=0; i*4 < item.size; i++) {
      item.buf[i] = x*32 + i;
    }
    s.pushItem(item);
  }


  s.finish();
  s.wait();
       
  verifyMD5(correct);
}
