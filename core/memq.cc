#include "memq.h"
#include <malloc.h>
#include <stdio.h>
#include <pthread.h>
#include "core.h"

// ##### QItem:  Single buffer, aligned/multiple on/of BLOCK_SIZE
QItem::QItem (unsigned int msize)
{
  orig_buf = (unsigned char *)malloc(msize + BLOCK_SIZE);
  if((unsigned int)orig_buf % BLOCK_SIZE == 0)
	buf = orig_buf;
  else
	buf = (orig_buf + BLOCK_SIZE) - ((unsigned int)orig_buf % BLOCK_SIZE);
  next = NULL;
  size = 0;
  maxsize = msize;
}

QItem::~QItem() {
  free(orig_buf);
}



// ##### MItem: multichannel buffer for non-interleaved access
MItem::MItem (unsigned int chans, unsigned int msize)
{
  unsigned int i;

  bufs = (unsigned char **)malloc(chans * sizeof(void *));
  for(i=0;i< chans; i++){
    bufs[i] = (unsigned char *)malloc(msize);
  }
  next = NULL;
  size = msize;
  maxsize = msize;
  channels = chans;
}

MItem::~MItem() {
  unsigned int i;
  for(i=0;i < channels; i++){
    free(bufs[i]);
  }
  free(bufs);
}





// ##### MemQ: locking fifo queue with recycled buffer stack

MemQ::MemQ(unsigned int channels, unsigned int buf_size)
{
  this->buf_size = buf_size;
  this->channels = channels;
  head = tail = empty = NULL;
  pthread_mutex_init(&lock, NULL);  // locks the main queue
  pthread_mutex_init(&mtlock, NULL); // locks the QItem pool (a stack in this implementation)
}


MemQ::~MemQ() {
  while(head != NULL) {
	delete(getHead());
  }
  while(empty != NULL) {
	delete(getEmpty());
  }
}

MItem *MemQ::getEmpty()
{
  MItem *foo;
    
  pthread_mutex_lock(&mtlock);
  if (empty == NULL) {
	foo = new MItem(channels, buf_size);
  }
  else {	
	foo = empty;
	empty = foo->next;
	foo->next = NULL;
  }
  pthread_mutex_unlock(&mtlock);
  return foo;
}

void MemQ::returnEmpty(MItem *mt) {
  pthread_mutex_lock(&mtlock);
  if (empty != NULL) {
    delete(mt);
    mt = empty;
  }
  empty = mt;
  pthread_mutex_unlock(&mtlock);
}

MItem *MemQ::getHead() {
  MItem *foo;
    
  pthread_mutex_lock(&lock);
  foo = head;
  if (foo != NULL) {
	head = foo->next;
	foo->next = NULL;
  }
  if (foo == tail) {
	tail = NULL;
  }
  pthread_mutex_unlock(&lock);
  return foo;
}

void MemQ::putTail(MItem *i) {
  pthread_mutex_lock(&lock);
  if (head == NULL) {
	head = tail = i;
  }
  else {
	tail->next = i;
	tail = i;
  }
  pthread_mutex_unlock(&lock);
}



