#include "imageThread.h"
#include <iostream>


enum Colors {
	RED = 0,
    GREEN,
    BLUE,
    ALPHA	
};

unsigned char uCharMax(unsigned char a, unsigned char b) {
    return ((a > b) ? a : b);
}

void setWindowEdges(long& startR, long& startC, long& endR, long& endC, long targetR, long targetC, long imgHeight, long imgWidth) {
    startR = targetR - (winSize / 2);
    startC = targetC - (winSize / 2);
    endR = targetR + (winSize / 2);
    endC = targetC + (winSize / 2);
    // handle case of even winSize by left bias (assuming target pixel is on the left half of window)
    if (winSize % 2 == 0) {
        ++startR;
        ++startC;
    }
    // bound checking
    if (startR < 0)
        startR = 0;
    if (startC < 0)
        startC = 0;
    // result will be negative if overflow occurs
    if (endR < 0 || endR >= imgHeight)
        endR = imgHeight - 1;
    if (endC < 0 || endC >= imgWidth)
        endC = imgHeight - 1;
}

// function to find the difference in the highest and lowest gray pixels in an image
long focusWindow(long targetR, long targetC, const unsigned char* currentImage, unsigned int perRow, unsigned int perPixel, long imgHeight, long imgWidth) {
    unsigned char min, max, gray;
    min = 255;
    max = gray = 0;
    // calculate edges of window
    long startR, startC, endR, endC, index;
    setWindowEdges(startR, startC, endR, endC, targetR, targetC, imgHeight, imgWidth);

    // find min and max gray pixel
    for (long i = startR; i <= endR; ++i) {
        for (long j = startC; j <= endC; ++j) {
            index = i * perRow + j * perPixel;
            // chose this gray method instead of averaging so I don't have to think about overflowing
            gray = uCharMax(uCharMax(currentImage[index + RED], currentImage[index + GREEN]), currentImage[index + BLUE]);
            if (gray < min)
                min = gray;
            if (gray > max)
                max = gray;
        }
    }
    return max - min;
}

// function to write out whole focus window 
void writeWindow(unsigned char* out, long targetR, long targetC, const unsigned char* in, unsigned int perRow, unsigned int perPixel, long imgHeight, long imgWidth) {
    // calculate edges of window
    long startR, startC, endR, endC, index;
    setWindowEdges(startR, startC, endR, endC, targetR, targetC, imgHeight, imgWidth);
    for (long i = startR; i <= endR; ++i) {
        for (long j = startC; j <= endC; ++j) {
            index = i * perRow + j * perPixel;
            // adjust the RED, GREEN, BLUE
            for (int k = 0; k < ALPHA; ++k) {
                // if black
                if (!out[index + k])
                    out[index + k] = in[index + k];
                else
                    out[index + k] = 0.5 * in[index + k] + 0.5 * out[index + k];
            }
            out[index + ALPHA] = in[index + ALPHA];
        }
    }
}

void* imageThreadFunc(void* arg) {
    // convert void* arg
    ImageThread* imageInfo = static_cast<ImageThread*>(arg);

    long maxDiff, diffIndex, tempDiff, randRow, randCol;
    // alias variables
    unsigned char* outPixels = static_cast<unsigned char*>(imageInfo->outputImage->raster);
    const unsigned char* inPixels = nullptr;
    for (std::size_t i = 0; i < 4 * (imageInfo->outputImage->height * imageInfo->outputImage->width) / numThreads; ++i) {
        // reset maximum difference and what image it is locatted in
        maxDiff = diffIndex = tempDiff = 0;
        // create generator using seed
        std::mt19937_64 generator(rd());
        // create distribution in range [min, max]
        std::uniform_int_distribution<long> rowDist(0, imageInfo->outputImage->height - 1);
        std::uniform_int_distribution<long> colDist(0, imageInfo->outputImage->width - 1);
        randRow = rowDist(generator);
        randCol = colDist(generator);
        // image currently looking at in image stack
        for (std::size_t k = 0; k < imageInfo->imageStack->size(); ++k) {
            inPixels = static_cast<const unsigned char*>(imageInfo->imageStack->at(k)->raster);
            tempDiff = focusWindow(randRow, randCol, inPixels, imageInfo->imageStack->at(k)->bytesPerRow, imageInfo->imageStack->at(k)->bytesPerPixel, imageInfo->imageStack->at(k)->height, imageInfo->imageStack->at(k)->width);
            if (tempDiff > maxDiff) {
                maxDiff = tempDiff;
                diffIndex = k;
            }
        }
        inPixels = static_cast<const unsigned char*>(imageInfo->imageStack->at(diffIndex)->raster);
        pthread_mutex_lock(imageInfo->lock);
        // write out focus window
        writeWindow(outPixels, randRow, randCol, inPixels, imageInfo->imageStack->at(diffIndex)->bytesPerRow, imageInfo->imageStack->at(diffIndex)->bytesPerPixel, imageInfo->imageStack->at(diffIndex)->height, imageInfo->imageStack->at(diffIndex)->width);
        pthread_mutex_unlock(imageInfo->lock);
    }
    imageInfo->status = DONE;
    return nullptr;
}
