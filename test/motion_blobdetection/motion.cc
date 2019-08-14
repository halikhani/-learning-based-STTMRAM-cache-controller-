#include <math.h>
#include <omp.h>
#include <iostream>
#include <string>
#include "image_processing.h"

//AMHM Start
//double ber[8]={0.001,0.001,0.001,0.001,0.001,0,0,0}; // FI Uniform 0.001 sometimes crash!
double ber[8]={0.01,0.01,0.01,0.01,0,0,0,0}; // FI Uniform 0.01 sometimes crash!
//double ber[8]={0,0,0,0,0.0001,0.000001,0.00000001,0.000000001};
//double ber[8]={0,0,0,0,0,0,0,0}; // Golden!
//AMHM End

int main(int argc, char **argv) {
  sImage originalImage, outputImage, greyImage, motionImage, erosionImage, edgeImage, bgImage, blobImage, bgGreyImage;
  
  if(argc!=4) {
    std::cout << "USAGE: " << argv[0] << " frame1.bmp frame2.bmp output.bmp" << std::endl;
    return 0;
  }
  readImage(argv[1], &bgImage);
  readImage(argv[2], &originalImage);

  //setup intermediated and final images
  initImage(&bgGreyImage,bgImage.rows,bgImage.cols,1, NULL);
  //AMHM Start
  AMHM_approx((long long int)&(bgGreyImage.data[0]), (long long int) (&bgGreyImage.data[0] + sizeof(unsigned char)*bgImage.rows*bgImage.cols*1 - 1));
  AMHM_qual(ToUnsignedInt(ber[1]));
  //AMHM End
  initImage(&outputImage,bgImage.rows,bgImage.cols,bgImage.depth, bgImage.header);
  //AMHM Start
  AMHM_approx((long long int)&(outputImage.data[0]), (long long int) (&outputImage.data[0] + sizeof(unsigned char)*bgImage.rows*bgImage.cols*bgImage.depth - 1));
  AMHM_qual(ToUnsignedInt(ber[2]));
  //AMHM End
  initImage(&greyImage,bgImage.rows,bgImage.cols,1, NULL);
  //AMHM Start
  AMHM_approx((long long int)&(greyImage.data[0]), (long long int) (&greyImage.data[0] + sizeof(unsigned char)*bgImage.rows*bgImage.cols*1 - 1));
  AMHM_qual(ToUnsignedInt(ber[3]));
  //AMHM End
  initImage(&motionImage,bgImage.rows,bgImage.cols,1, NULL);
  //AMHM Start
  AMHM_approx((long long int)&(motionImage.data[0]), (long long int) (&motionImage.data[0] + sizeof(unsigned char)*bgImage.rows*bgImage.cols*1 - 1));
  AMHM_qual(ToUnsignedInt(ber[4]));
  //AMHM End
  initImage(&blobImage,bgImage.rows,bgImage.cols,1, NULL);
  //AMHM Start
  AMHM_approx((long long int)&(blobImage.data[0]), (long long int) (&blobImage.data[0] + sizeof(unsigned char)*bgImage.rows*bgImage.cols*1 - 1));
  AMHM_qual(ToUnsignedInt(ber[5]));
  //AMHM End
  initImage(&erosionImage,bgImage.rows,bgImage.cols,1, NULL);
  //AMHM Start
  AMHM_approx((long long int)&(erosionImage.data[0]), (long long int) (&erosionImage.data[0] + sizeof(unsigned char)*bgImage.rows*bgImage.cols*1 - 1));
  AMHM_qual(ToUnsignedInt(ber[6]));
  //AMHM End
  initImage(&edgeImage,bgImage.rows,bgImage.cols,1, NULL);
  //AMHM Start
  AMHM_approx((long long int)&(edgeImage.data[0]), (long long int) (&edgeImage.data[0] + sizeof(unsigned char)*bgImage.rows*bgImage.cols*1 - 1));
  AMHM_qual(ToUnsignedInt(ber[7]));
  //AMHM End

  rgb2grey(&bgImage,&bgGreyImage);
  rgb2grey(&originalImage,&greyImage);
  motion_detection(&greyImage, &bgGreyImage, &motionImage);  
  erosion_filter(&motionImage, &erosionImage);
  //sobel_edge_detection(&erosionImage, &edgeImage);
  blob_detection(&erosionImage, &blobImage);
  overlap_features_filter(&originalImage, &blobImage, 100, &outputImage);
  //grey2rgb(&outputImage, &blobImage);
  
  //writing file
  writeImage(argv[3], &outputImage);
  //AMHM Start
  AMHM_accurate((long long int)&(bgGreyImage.data[0]));
  AMHM_accurate((long long int)&(outputImage.data[0]));
  AMHM_accurate((long long int)&(greyImage.data[0]));
  AMHM_accurate((long long int)&(motionImage.data[0]));
  AMHM_accurate((long long int)&(blobImage.data[0]));
  AMHM_accurate((long long int)&(erosionImage.data[0]));
  AMHM_accurate((long long int)&(edgeImage.data[0]));
  AMHM_accurate((long long int)&(originalImage.data[0]));
  AMHM_accurate((long long int)&(bgImage.data[0]));
  //AMHM End
  
  deleteImage(&greyImage);
  deleteImage(&originalImage);
  deleteImage(&outputImage);
  deleteImage(&motionImage);
  deleteImage(&blobImage);
  deleteImage(&erosionImage);
  deleteImage(&edgeImage);
  deleteImage(&bgImage);
  deleteImage(&bgGreyImage);
  return 0;
 
}
