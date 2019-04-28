#include "imageThread.h"
#include <iostream>

void* imageThreadFunc(void* arg) {
    ImageThread* imageInfo = static_cast<ImageThread*>(arg);
    std::cout << imageInfo->threadID << std::endl;
    std::cout << imageInfo->imageStack->size() << std::endl;
    std::cout << imageInfo->outputImage << std::endl;
    std::cout << imageInfo->startRow << std::endl;
    std::cout << imageInfo->endRow << std::endl;
    std::cout << '\n' << std::endl;
    imageInfo->status = DONE;
    return nullptr;
}