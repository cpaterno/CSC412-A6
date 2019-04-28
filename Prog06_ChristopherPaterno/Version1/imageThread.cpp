#include "imageThread.h"
#include <iostream>


enum Colors {
	RED = 0,
    GREEN,
    BLUE,
    ALPHA	
};

long slidingWindow(long targetR, long targetC, const unsigned char* currentImage, ) {

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
                tempDiff = slidingWindow(i, j, inPixels, imageInfo->imageStack->at(k)->);
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
            outPixels[pixelIndex + ALPHA] = inPixels[pixelIndex + ALPHA];

        }
    }
    imageInfo->status = DONE;
    return nullptr;
}

