/*
This class implements a queue of buffer objects
You initiliaze it with how large the allocated buffers are supposed to be
To use it, call getEmpty to get an emtpy buffer object.  Put something in it's buffer and set the size, then call putTail on the Q.
Consumers call getHead then use returnEmpty when they are done with the buffer object
buffer objects are recycled, new ones are created if getEmpty is called and there are now items in the empty list

*/
#include <pthread.h>

class QItem {
public:
    unsigned char *buf;
    QItem *next;
    unsigned int size;
    unsigned int maxsize;
    QItem(unsigned int max_size);
    ~QItem();
protected:
    unsigned char*orig_buf;
};

class MItem {
 public:
  unsigned char **bufs;
  MItem *next;
  unsigned int size;
  unsigned int channels;
  unsigned int maxsize;
  MItem(unsigned int chans, unsigned int max_size);
  ~MItem();
};



class MemQ {
public:
    MItem *getEmpty();
    void returnEmpty(MItem *mt);
    MItem *getHead();
    void putTail(MItem *i);
    MemQ(unsigned int channels, unsigned int buf_size);
    ~MemQ();
protected:
    MItem *head;
    MItem *tail;
    MItem *empty;
    unsigned int buf_size;
    unsigned int channels;
    pthread_mutex_t lock;
    pthread_mutex_t mtlock;
};

