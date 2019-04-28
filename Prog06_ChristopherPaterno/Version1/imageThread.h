#ifndef	IMAGE_THREAD_H
#define	IMAGE_THREAD_H

#include <vector>
#include <pthread.h>
#include "rasterImage.h"

const std::size_t numThreads = 6;

enum ThreadStatus {
	RUNNING = 0,
    DONE,
    JOINED	
};

struct ImageThread {
    pthread_t threadID;
    const std::vector<ImageStruct*>* imageStack;
    ImageStruct* outputImage;
    std::size_t startRow;
    std::size_t endRow;
    ThreadStatus status;
};

// upon entering this function cast the arg to ImageThread*
void* imageThreadFunc(void* arg);

#endif