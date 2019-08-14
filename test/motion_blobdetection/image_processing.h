#ifndef _IMAGE_PROCESSING_H_
#define _IMAGE_PROCESSING_H_

//AMHM Start
#include "../../include/sim_api.h"
#define ToUnsignedInt(X) *((unsigned long long*)(&X))
extern double ber[];
//AMHM End

typedef struct {
  int rows;
  int cols;
  int depth;
  unsigned char* header;
  unsigned char* data;
} sImage;

void initImage(sImage* image, int rows, int cols, int depth, unsigned char* headar);
void deleteImage(sImage* image);
void readImage(const char* filename, sImage* image);
void writeImage(const char* filename, sImage* image);

void rgb2grey (sImage* originalImage, sImage* greyImage);
void sobel_edge_detection (sImage* greyImage, sImage* edgeImage);
void grey2rgb (sImage* originalImage, sImage* edgeImage);
void template_matching (sImage* image, sImage* templImage);
void image_twisting (sImage* image, sImage* outputImage, float factor);
void gaussian_filter (sImage* originalImage, sImage* finalImage, float sigma);
void erosion_filter(sImage* originalImage, sImage* outputImage);
void motion_detection(sImage* originalImage, sImage* backgroundImage, sImage* outputImage);
void blob_detection(sImage* originalImage, sImage* blobImage);
void overlap_features_filter(sImage* originalImage, sImage* featuresImage, int threshold, sImage* outputImage);

#endif
