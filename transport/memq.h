#include <semaphore.h>

/*
  This class implements a queue of buffer objects
  You initiliaze it with how large the allocated buffers are supposed to be
  To use it, call getEmpty to get an emtpy buffer object.  Put something in it's buffer and set the size, then call putTail on the Q.
  Consumers call getHead then use returnEmpty when they are done with the buffer object
  buffer objects are recycled, new ones are created if getEmpty is called and there are now items in the empty list

*/
#include <pthread.h>
#include <FLAC/all.h>
#include <time.h>

#ifndef __MEMQ__
#define __MEMQ__

class QItem {
 public:
  FLAC__int32 *buf;
  unsigned int frames;
  unsigned int bufsize;
  QItem *next;
  QItem(unsigned int max_size);
  ~QItem();
};


class MemQ {
 public:
  QItem *getEmpty();
  void returnEmpty(QItem *mt);
  QItem *getHead(struct timespec *wait);
  unsigned int getSize();
  void putTail(QItem *i);
  MemQ(unsigned int max_size, unsigned int buf_size);
  ~MemQ();

 protected:
  QItem *head;
  QItem *tail;
  QItem *empty;
  sem_t sem;
  unsigned int buf_size;
  unsigned int max_size;
  unsigned int size;
  pthread_mutex_t lock;
  pthread_mutex_t mtlock;
  sem_t asem;
};

#endif
