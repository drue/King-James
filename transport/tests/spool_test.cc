#include <stdlib.h>

#include "gtest/gtest.h"
#include "FLAC++/metadata.h"
#include "spool.h"
#include "memq.h"

class SpoolTest : public ::testing::Test {
public:
  char tmpl[80];
  char *f;

  virtual void SetUp() {
    strncpy(tmpl, "/tmp/SpoolTest.XXXXXX", sizeof(tmpl));
    f = mktemp(tmpl);
  }
};

TEST_F(SpoolTest, DeallocUnused) {
  Spool s(128*5, 128, 24, 44100, 2);
}

TEST_F(SpoolTest, Null) {

  Spool s(5, 128, 24, 48000, 2);
  const char *emptymd5 = "\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42\x7e";

  s.start(f);
  s.finish();
  s.wait();
  FLAC::Metadata::StreamInfo si;
  FLAC::Metadata::get_streaminfo(f, si);
  const unsigned char *sum = si.get_md5sum();
  for(int i=0;i<16;i++) {
    ASSERT_EQ(emptymd5[i], sum[i]);
  }
}

TEST_F(SpoolTest, OneItem) {
  FLAC__uint32 data;  
  Spool s(128*5, 128, 24, 48000, 2);

  s.start(f);
  QItem *item = s.getEmpty();
  int i;
  for(i=0;i<item->bufsize;i++) {
    item->buf[i] = i;
  }
  item->size = i;

  s.pushItem(item);

  s.finish();
  s.wait();
  // test file
}
