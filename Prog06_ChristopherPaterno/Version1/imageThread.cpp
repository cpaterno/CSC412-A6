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

long slidingWindow(long targetR, long targetC, const unsigned char* currentImage, unsigned int perRow, unsigned int perPixel, long imgHeight, long imgWidth) {
    unsigned char min, max, gray;
    min = 255;
    max = gray = 0;
    long startR, startC, endR, endC, index;
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

    for (long i = startR; i <= endR; ++i) {
        for (long j = startC; j <= endC; ++j) {
            index = i * perRow + j * perPixel;
            gray = uCharMax(uCharMax(currentImage[index + RED], currentImage[index + GREEN]), currentImage[index + BLUE]);
            if (gray < min)
                min = gray;
            if (gray > max)
                max = gray;
        }
    }
    return max - min;
}

void* imageThreadFunc(void* arg) {
    // convert void* arg
    ImageThread* imageInfo = static_cast<ImageThread*>(arg);

    long maxDiff, diffIndex, tempDiff, pixelIndex;
    // alias variables
    unsigned char* outPixels = static_cast<unsigned char*>(imageInfo->outputImage->raster);
    const unsigned char* inPixels = nullptr;
    // row of current pixel
    for (std::size_t i = imageInfo->startRow; i < imageInfo->endRow; ++i) {
        // column of current pixel
        for (std::size_t j = 0; j < imageInfo->outputImage->width; ++j) {
            // reset maximum difference and what image it is locatted in
            maxDiff = diffIndex = tempDiff = 0;
            pixelIndex = i * imageInfo->outputImage->bytesPerRow + j * imageInfo->outputImage->bytesPerPixel;
            // image currently looking at in image stack
            for (std::size_t k = 0; k < imageInfo->imageStack->size(); ++k) {
                inPixels = static_cast<const unsigned char*>(imageInfo->imageStack->at(k)->raster);
                tempDiff = slidingWindow(i, j, inPixels, imageInfo->imageStack->at(k)->bytesPerRow, imageInfo->imageStack->at(k)->bytesPerPixel, imageInfo->imageStack->at(k)->height, imageInfo->imageStack->at(k)->width);
                if (tempDiff > maxDiff) {
                    maxDiff = tempDiff;
                    diffIndex = k;
                }
            }
            inPixels = static_cast<const unsigned char*>(imageInfo->imageStack->at(diffIndex)->raster);
            // write to output image target pixel using values from the sharpest image's target pixel
            outPixels[pixelIndex + RED] = inPixels[pixelIndex + RED];
            outPixels[pixelIndex + GREEN] = inPixels[pixelIndex + GREEN];
            outPixels[pixelIndex + BLUE] = inPixels[pixelIndex + BLUE];
            //outPixels[pixelIndex + ALPHA] = inPixels[pixelIndex + ALPHA];

        }
    }
    imageInfo->status = DONE;
    return nullptr;
}

