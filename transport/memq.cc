#include <malloc.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#include "const.h"
#include "memq.h"


// ##### QItem:  Single buffer, aligned/multiple on/of BLOCK_SIZE
QItem::QItem (unsigned int msize)
{
  buf = (FLAC__int32 *)malloc(msize);
  next = NULL;
  size = 0;
  bufsize = msize;
}

QItem::~QItem() {
  free(buf);
}




// ##### MemQ: locking fifo queue with recycled buffer stack

MemQ::MemQ(unsigned int max_size, unsigned int buf_size)
{
  this->buf_size = buf_size;
  this->max_size = max_size;
  head = tail = empty = NULL;
  size = 0;
  pthread_mutex_init(&lock, NULL);  // locks the main queue
  pthread_mutex_init(&mtlock, NULL); // locks the QItem pool (a stack in this implementation)
  sem_init(&asem, 0, 0);
}


MemQ::~MemQ() {
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts) ;

  while(head != NULL) {
	delete(getHead(&ts));
  }
  while(empty != NULL) {
	delete(getEmpty());
  }
}

unsigned int MemQ::getSize() {
  unsigned int s;

  pthread_mutex_lock(&lock);
  s = this->size;
  pthread_mutex_unlock(&lock);

  return s;
}

unsigned int MemQ::bufSize() {
  return buf_size;
}

QItem *MemQ::getEmpty()
{
  QItem *foo;
    
  pthread_mutex_lock(&mtlock);
  if (empty == NULL) {
	foo = new QItem(buf_size);
  }
  else {	
	foo = empty;
	empty = foo->next;
	foo->next = NULL;
  }
  pthread_mutex_unlock(&mtlock);
  return foo;
}

void MemQ::returnEmpty(QItem *mt) {
  pthread_mutex_lock(&mtlock);
  if (empty != NULL) {
    delete(mt);
    mt = empty;
  }
  empty = mt;
  empty->size = 0;
  pthread_mutex_unlock(&mtlock);
}

QItem *MemQ::getHead(struct timespec *wait) {
  QItem *foo;
    
  sem_timedwait(&asem, wait);
  pthread_mutex_lock(&lock);
  foo = head;
  if (foo != NULL) {
	head = foo->next;
	foo->next = NULL;
    size -= 1;
  }
  if (foo == tail) {
	tail = NULL;
  }
  pthread_mutex_unlock(&lock);
  return foo;
}

void MemQ::putTail(QItem *i) {
  QItem *foo;

  pthread_mutex_lock(&lock);

  if (head == NULL) {
	head = tail = i;
  }
  else {
	tail->next = i;
	tail = i;
  }

  tail->next = NULL;
  size += 1;

  if (size <= max_size)
    sem_post(&asem);

  while(size > max_size) {
    foo = head;
    head = foo->next;
    foo->next = NULL;
    size -= 1;
    returnEmpty(foo);
  }
    
  pthread_mutex_unlock(&lock);
}


