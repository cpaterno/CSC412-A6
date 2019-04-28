#include <stdio.h>
#include <stdlib.h>
//
#include "rasterImage.h"

ImageStruct::ImageStruct(unsigned int theWidth, unsigned int theHeight, ImageType theType,
						 unsigned int wordSizeRowPadding)
		:	width(theWidth),
			height(theHeight),
			type(theType)
{

	switch (type)
	{
		case RGBA32_RASTER:
		bytesPerPixel = 4;
		break;
		
		case GRAY_RASTER:
		bytesPerPixel = 1;
		break;
		
		case DEEP_GRAY_RASTER:
		bytesPerPixel = 2;
		break;
		
		case FLOAT_RASTER:
		bytesPerPixel = sizeof(float);
		break;
		
		default:
			break;
	}

	bytesPerRow = ((bytesPerPixel * width + wordSizeRowPadding - 1)/wordSizeRowPadding)*wordSizeRowPadding;

	unsigned int effectiveWidth = bytesPerRow / bytesPerPixel;
	raster = (void*) calloc(height*effectiveWidth, bytesPerPixel);

	switch (type)
	{
		case RGBA32_RASTER:
		case GRAY_RASTER:
		{
			unsigned char* r1D = (unsigned char*) raster;
			unsigned char** r2D = (unsigned char**) calloc(height, sizeof(unsigned char*));
			raster2D = (void*) r2D;
			for (unsigned int i=0; i<height; i++)
				r2D[i] = r1D + i*bytesPerRow;
		}
		break;
		
		case DEEP_GRAY_RASTER:
		{
			unsigned short* r1D = (unsigned short*) raster;
			unsigned short** r2D = (unsigned short**) calloc(height, sizeof(unsigned short*));
			raster2D = (void*) r2D;
			for (unsigned int i=0; i<height; i++)
				r2D[i] = r1D + i*effectiveWidth;
		}
		break;
		
		case FLOAT_RASTER:
		{
			float* r1D = (float*) raster;
			float** r2D = (float**) calloc(height, sizeof(float*));
			raster2D = (void*) r2D;
			for (unsigned int i=0; i<height; i++)
				r2D[i] = r1D + i*effectiveWidth;
		}
		break;

		default:
			break;
	}

}

ImageStruct::~ImageStruct(void) {
	switch (type) {
		case RGBA32_RASTER:
		case GRAY_RASTER:
			delete []((unsigned char*)raster);
			delete []((unsigned char**)raster2D);
			break;
		
		case DEEP_GRAY_RASTER:
			delete []((unsigned short*)raster);
			delete []((unsigned short**)raster2D);
			break;
		
		case FLOAT_RASTER:
			delete []((float*)raster);
			delete []((float**)raster2D);
			break;
		
		default:
			break;
	}
}

