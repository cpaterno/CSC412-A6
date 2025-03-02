#ifndef	IMAGE_THREAD_H
#define	IMAGE_THREAD_H

#include <vector>
#include <random>
#include <pthread.h>
#include "rasterImage.h"

const std::size_t numThreads = 16;
const std::size_t winSize = 11;
extern std::random_device rd;


enum ThreadStatus {
	RUNNING = 0,
    DONE,
    JOINED	
};

struct ImageThread {
    pthread_t threadID;
    const std::vector<ImageStruct*>* imageStack;
    ImageStruct* outputImage;
    ThreadStatus status;
    pthread_mutex_t* lock;
};

// upon entering this function cast the arg to ImageThread*
void* imageThreadFunc(void* arg);

#endif