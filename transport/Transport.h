#include <alsa/asoundlib.h>

#include <pthread.h>
#include <semaphore.h>

#include "port.h"
#include "memq.h"

extern "C" {
#include <FLAC/stream_encoder.h>
}


class Transport {
public:
    virtual void stop();
    virtual void startRecording(char *path);
};

class AlsaTPort: public Transport {
protected:
    pthread_t cthread, sthread;
    sem_t asem;
    MemQ *Q;
    FLAC__StreamEncoder *encoder;
    int stop_flag, tstop, finished;
    int bits_per_sample, sample_rate;
    long amax, bmax, aavg, bavg;
    APort *pla;
    APort *cap;
    unsigned int aligned_buffer_size;
    QItem *wbuffer;
    char *filename;
    int fd;
    pthread_mutex_t maxlock;

    static void doCapture(void *foo);
    void doReturn();
    static void doSave(void *foo);
    static FLAC__StreamEncoderWriteStatus write_callback(const FLAC__StreamEncoder *encoder, 
                                                         const FLAC__byte buffer[],
                                                         size_t bytes, unsigned samples,
                                                         unsigned current_frame, void *client_data);
    static void metadata_callback(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data);

public:
    sem_t finished_sem;
    AlsaTPort(int bits_per_sample, int sample_rate);
    virtual ~AlsaTPort();
    void startRecording(char *path);
    virtual void stop();
    virtual void wait();
    virtual int gotSignal();

    long getmaxa();
    long getmaxb();
    void resetmax();
};
