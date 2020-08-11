#include <math.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <string.h>
#include"cache-sim/cache_model.h"
#include "sttmram.h"

#define CONFIG_FN "params3.cfg"
#define RANDOM_CACHE_INIT true

#define ORIGINAL_IMAGEF 0
#define ORIGINAL_IMAGEB 1
#define GREY_IMAGEF 2 
#define GREY_IMAGEB 3 
#define MOTION_IMAGE 4
#define EROSION_IMAGE 5
#define BLOB_IMAGE 6
#define MAX_QL 7

//motion detection macros
#define MD_THRESHOLD 30
#define EROSION_MAP_SIZE 7 /*must be an odd number*/
#define FG 255
#define BG 0
#define ER_THRESHOLD 0.3

typedef struct {
  int rows;
  int cols;
  int depth;
  int size;
  unsigned char* header;
  unsigned char* data;
} sImage;

void initImage(sImage* image, int rows, int cols, int depth, unsigned char* headar);
void deleteImage(sImage* image);
void readImage(const char* filename, sImage* image);
void writeImage(const char* filename, sImage* image);
void rgb2grey (sImage* originalImage, sImage* greyImage, bool background);
void grey2rgb (sImage* originalImage, sImage* edgeImage);
void erosion_filter(sImage* originalImage, sImage* outputImage);
void motion_detection(sImage* originalImage, sImage* backgroundImage, sImage* outputImage);
int** blob_detection(sImage* originalImage, sImage* blobImage, int* num_of_bbs, bool output);
void overlap_features_filter(sImage* originalImage, sImage* featuresImage, int threshold, sImage* outputImage);

#define TARGET_CACHE_LEVEL 2 
CacheModel cm(CONFIG_FN);
long long oldBlockBaseAddress;
long long newBlockBaseAddress;
bool miss;
long long blockSize;
        
int main(int argc, char **argv) {
  sImage originalImage, outputImage, outputImage2, greyImage, motionImage, erosionImage, bgImage, blobImage, bgGreyImage;
  int i, bbs_num;
  int** bb_coords;
  int seed;
  bool output=false;
  
  if(argc<4 || argc>6) {
    std::cout << "USAGE: " << argv[0] << " frame1.bmp frame2.bmp sttmram_config [seed] [output]" << std::endl<< std::endl;
    std::cout << "sttmram_config is a list of QLs separated by a -."  << std::endl;
    std::cout << "This application has 4 steps so it requires 4 different QLs."  << std::endl;
    std::cout << "DO NOTE: for simplicity there is no check on wrong values in the configuration."  << std::endl;
    std::cout << "Specify the last parameter (any string is fine) to save output bmps" << std::endl;
    return 1;
  }
  seed=0;
  if(argc>=5)
    seed=atoi(argv[4]);
  if(argc==6)
    output=true;

  we_setup(argv[3], seed, RANDOM_CACHE_INIT);
  blockSize = cm.getBlockSize(TARGET_CACHE_LEVEL);

  readImage(argv[1], &bgImage);
  readImage(argv[2], &originalImage);

  //setup intermediated and final images
  initImage(&bgGreyImage,bgImage.rows,bgImage.cols,1, NULL);
  initImage(&greyImage,bgImage.rows,bgImage.cols,1, NULL);
  initImage(&motionImage,bgImage.rows,bgImage.cols,1, NULL);
  initImage(&blobImage,bgImage.rows,bgImage.cols,1, NULL);
  initImage(&erosionImage,bgImage.rows,bgImage.cols,1, NULL);
  if(output){
    initImage(&outputImage,bgImage.rows,bgImage.cols,bgImage.depth, bgImage.header);
    initImage(&outputImage2,bgImage.rows,bgImage.cols,bgImage.depth, bgImage.header);
  }
  
  rgb2grey(&bgImage,&bgGreyImage, true);
  rgb2grey(&originalImage,&greyImage, false);
  motion_detection(&greyImage, &bgGreyImage, &motionImage);  
  erosion_filter(&motionImage, &erosionImage);
  bb_coords = blob_detection(&erosionImage, &blobImage, &bbs_num, output);
  if(output){
    writeImage("output0.bmp", &originalImage);
    writeImage("output1.bmp", &bgImage);
    grey2rgb(&outputImage2, &greyImage);
    writeImage("output2.bmp", &outputImage2);
    grey2rgb(&outputImage2, &bgGreyImage);
    writeImage("output3.bmp", &outputImage2);
    grey2rgb(&outputImage2, &motionImage);
    writeImage("output4.bmp", &outputImage2);
    grey2rgb(&outputImage2, &erosionImage);
    writeImage("output5.bmp", &outputImage2);
    grey2rgb(&outputImage2, &blobImage);
    writeImage("output6.bmp", &outputImage2);
    overlap_features_filter(&originalImage, &blobImage, 100, &outputImage);
    writeImage("output7.bmp", &outputImage);
  }


  for(i=0; i<bbs_num; i++)
    std::cout << bb_coords[i][0] << " " << bb_coords[i][1] << " -> " << bb_coords[i][2] << " " << bb_coords[i][3] << std::endl;
  for(i=0; i<bbs_num; i++)
    free(bb_coords[i]);
  free(bb_coords);

  std::cerr << "NOER: " << countnofault << std::endl 
            << "0to1: " << count0to1 << std::endl 
            << "1to0: " << count1to0 << std::endl;

  cm.printCacheStatus(stderr);

  deleteImage(&greyImage);
  deleteImage(&originalImage);
  deleteImage(&motionImage);
  deleteImage(&blobImage);
  deleteImage(&erosionImage);
  deleteImage(&bgImage);
  deleteImage(&bgGreyImage);
  if(output){
    deleteImage(&outputImage);
    deleteImage(&outputImage2);
  }
  return 0;
}

/*
 * Initializes the memory object
 */
void initImage(sImage* image, int rows, int cols, int depth, unsigned char* header){
  image->rows = rows; 
  image->cols = cols; 
  image->depth = depth;  
  image->size = rows*cols*depth; 
  image->data = (unsigned char *)malloc(sizeof(unsigned char)*rows*cols*depth);
  //memset(image->data, 0, sizeof(unsigned char)*rows*cols*depth); //DO NOTE: for debugging purposes we perform a memset to 0. TODO remove it
  if(header){
    image->header = (unsigned char *)malloc(sizeof(unsigned char)*54L);
    memcpy(image->header,header,54L);
  } else
    image->header = NULL;
}

/*
 * Deletes the memory object
 */
void deleteImage(sImage* image){
  if(image->header)
    free(image->header);
  if(image->data)
    free(image->data);
  image->rows = image->cols = image->depth = 0;
  image->header = image->data = NULL;
}

/*
 * Reads a 24bit RGB image from a file and saves it in a memory object (required memory is instantiated here)
 */
void readImage(const char* filename, sImage* image){
  FILE *bmpInput;
  int bits = 0;
  int fileSize = 0;
  int results;
  
  bmpInput = fopen(filename, "rb");  
  if(bmpInput){
  
    image->header = (unsigned char *)malloc(sizeof(unsigned char)*54L);
    results=fread(image->header, sizeof(char), 54L, bmpInput);
    if(results!=54L)
      printf("Bmp read error\n");
    //else
    //  printf("header read ok!\n");
    
    memcpy(&fileSize,&image->header[2],4);
    memcpy(&image->cols,&image->header[18],4);
    memcpy(&image->rows,&image->header[22],4);
    memcpy(&bits,&image->header[28],4);
    image->depth=3;
    image->size = image->rows*image->cols*image->depth; 

    
/*    printf("Width: %d\n", image->cols);
    printf("Height: %d\n", image->rows);
    printf("File size: %d\n", fileSize);
    printf("Bits/pixel: %d\n", bits);
  */  
    if(bits!=24 || fileSize!=image->rows*image->cols*image->depth+54){
      printf("Wrong image format in %s: accepted only 24 bit without padding!\n", filename);
      exit(1);
    }
      
    image->data = (unsigned char *)malloc(image->rows*image->cols*sizeof(unsigned char)*3);
    fseek(bmpInput, 54L, SEEK_SET);
    results=fread(image->data, sizeof(char), (image->rows*image->cols*image->depth), bmpInput);
    if(results != (image->rows*image->cols*image->depth))
      printf("Bmp read error\n");
    //else
    //  printf("data read ok %d!\n", results);
    //printf("File read\n");

    fclose(bmpInput);
  } else {
    printf("File not found: %s\n",filename);
    exit(1);  
  }
}

/*
 * Saves an image in a file
 */
void writeImage(const char* filename, sImage* image){
  FILE *bmpOutput;
  bmpOutput = fopen(filename, "wb");  
  if(bmpOutput){
    fwrite(image->header, sizeof(char), 54L, bmpOutput);
    fwrite(image->data, sizeof(char), (image->rows*image->cols*image->depth), bmpOutput);
    fclose(bmpOutput);
  } else {
    printf("File not opened: %s\n",filename);
    exit(1);  
  }
}

/*
 * Converts a grey-scale image in RGB format and saves it in another object. Memory has to be already instantiated
 */
void grey2rgb (sImage* outputImage, sImage* greyImage){
  int i=0;
  //#pragma omp parallel for 
  for(i = 0; i < (outputImage->rows*outputImage->cols); i++){
/*    cm.access((long long) (greyImage->data + i), false);
    cm.access((long long) (greyImage->data + i), false);
    cm.access((long long) (greyImage->data + i), false);

    cm.access((long long) (outputImage->data + i*3), false);
    cm.access((long long) (outputImage->data + i*3+1), false);
    cm.access((long long) (outputImage->data + i*3+2), false);*/

    *(outputImage->data + i*3) = *(greyImage->data +i);
    *(outputImage->data + i*3 +1) = *(greyImage->data +i);
    *(outputImage->data + i*3 +2) = *(greyImage->data +i);
  }
}

/*
 * Converts an RGB image in grey-scale format and saves it in another object. Memory has to be already instantiated
 */
void rgb2grey (sImage* originalImage, sImage* greyImage, bool background){
  int r = 0;
  int c = 0;
  unsigned char  redValue, greenValue, blueValue, grayValue;
      
  //#pragma omp parallel for private (blueValue, greenValue, redValue, grayValue) //default (shared)
  for(r=0; r<originalImage->rows; r++){
    for (c=0; c<originalImage->cols; c++){
      cm.access((long long) (originalImage->data + (r*originalImage->cols + c)*3), false);
      cm.getLastAccessStats(TARGET_CACHE_LEVEL, &miss, &newBlockBaseAddress, &oldBlockBaseAddress);
      if(miss)
        we_miss_block((long long) (originalImage->data), originalImage->size*sizeof(unsigned char), newBlockBaseAddress, oldBlockBaseAddress, blockSize, background?ORIGINAL_IMAGEB:ORIGINAL_IMAGEF);
       
      cm.access((long long) (originalImage->data + (r*originalImage->cols + c)*3+1), false);
      cm.getLastAccessStats(TARGET_CACHE_LEVEL, &miss, &newBlockBaseAddress, &oldBlockBaseAddress);
      if(miss)
        we_miss_block((long long) (originalImage->data), originalImage->size*sizeof(unsigned char), newBlockBaseAddress, oldBlockBaseAddress, blockSize, background?ORIGINAL_IMAGEB:ORIGINAL_IMAGEF);

      cm.access((long long) (originalImage->data + (r*originalImage->cols + c)*3+2), false);
      cm.getLastAccessStats(TARGET_CACHE_LEVEL, &miss, &newBlockBaseAddress, &oldBlockBaseAddress);
      if(miss)
        we_miss_block((long long) (originalImage->data), originalImage->size*sizeof(unsigned char), newBlockBaseAddress, oldBlockBaseAddress, blockSize, background?ORIGINAL_IMAGEB:ORIGINAL_IMAGEF);

      cm.access((long long) (greyImage->data + r*originalImage->cols +c), false);
      cm.getLastAccessStats(TARGET_CACHE_LEVEL, &miss, &newBlockBaseAddress, &oldBlockBaseAddress);
      if(miss)
        we_miss_block((long long) (greyImage->data), greyImage->size*sizeof(unsigned char), newBlockBaseAddress, oldBlockBaseAddress, blockSize, MAX_QL); // background?GREY_IMAGEB:GREY_IMAGEF);


      /*-----READ FIRST BYTE TO GET BLUE VALUE-----*/
      blueValue = *(originalImage->data + (r*originalImage->cols + c)*3);            
      /*-----READ NEXT BYTE TO GET GREEN VALUE-----*/
      greenValue = *(originalImage->data + (r*originalImage->cols +c)*3+1);            
      /*-----READ NEXT BYTE TO GET RED VALUE-----*/
      redValue = *(originalImage->data + (r*originalImage->cols +c)*3+2);            
      /*-----USE FORMULA TO CONVERT RGB VALUE TO GRAYSCALE-----*/
      grayValue = (unsigned char) (0.299*redValue + 0.587*greenValue + 0.114*blueValue);
      //*(greyImage->data + r*originalImage->cols +c) = grayValue;
      we_assign_uchar((greyImage->data + r*originalImage->cols +c), grayValue, background?GREY_IMAGEB:GREY_IMAGEF);
    }
  }    
}

/*
 * Performs the motion detection
 */
void motion_detection(sImage* originalImage, sImage* backgroundImage, sImage* outputImage){
  int r,c;
  //#pragma omp parallel for private(r, c) //default(shared) //shared(edgeImage, GX, GY, greyImage) //collapse(2)
  for(r=0; r<originalImage->rows; r++){
    for (c=0; c<originalImage->cols; c++){
      cm.access((long long) (originalImage->data +r*originalImage->cols+c), false);
      cm.getLastAccessStats(TARGET_CACHE_LEVEL, &miss, &newBlockBaseAddress, &oldBlockBaseAddress);
      if(miss)
        we_miss_block((long long) (originalImage->data), originalImage->size*sizeof(unsigned char), newBlockBaseAddress, oldBlockBaseAddress, blockSize, GREY_IMAGEF);
      
      
      cm.access((long long) (backgroundImage->data+r*originalImage->cols+c), false);
      cm.getLastAccessStats(TARGET_CACHE_LEVEL, &miss, &newBlockBaseAddress, &oldBlockBaseAddress);
      if(miss)
        we_miss_block((long long) (backgroundImage->data), backgroundImage->size*sizeof(unsigned char), newBlockBaseAddress, oldBlockBaseAddress, blockSize, GREY_IMAGEB);

      cm.access((long long) (outputImage->data+r*originalImage->cols+c), false);
      cm.getLastAccessStats(TARGET_CACHE_LEVEL, &miss, &newBlockBaseAddress, &oldBlockBaseAddress);
      if(miss)
        we_miss_block((long long) (outputImage->data), outputImage->size*sizeof(unsigned char), newBlockBaseAddress, oldBlockBaseAddress, blockSize, MAX_QL); // MOTION_IMAGE);

      //outputImage->data[r*originalImage->cols+c]=(abs(originalImage->data[r*originalImage->cols+c]-backgroundImage->data[r*originalImage->cols+c])>MD_THRESHOLD)?FG:BG;
      unsigned char val = (abs(originalImage->data[r*originalImage->cols+c]-backgroundImage->data[r*originalImage->cols+c])>MD_THRESHOLD)?FG:BG;
      we_assign_uchar(outputImage->data+r*originalImage->cols+c, val, MOTION_IMAGE);
    }
  }  
}

/*
 * Performs the erosion filter on the motion detection image
 */
void erosion_filter(sImage* originalImage, sImage* outputImage){
  int r,c, r1, c1, marker, count;
  //#pragma omp parallel for private(r, c, r1, c1, marker, count) //default(shared) //shared(edgeImage, GX, GY, greyImage) //collapse(2)
  for(r=0; r<originalImage->rows; r++){
    for (c=0; c<originalImage->cols; c++){
      marker=0;
      count=0;
      for(r1=(r-(EROSION_MAP_SIZE-1)/2>0)?(r-(EROSION_MAP_SIZE-1)/2):0; r1<r+(EROSION_MAP_SIZE-1)/2 && r1<originalImage->rows; r1++){
        for(c1=(c-(EROSION_MAP_SIZE-1)/2>0)?c-(EROSION_MAP_SIZE-1)/2:0; c1<c+(EROSION_MAP_SIZE-1)/2 && c1<originalImage->cols; c1++){
          count++;
          cm.access((long long) (originalImage->data+r1*originalImage->cols+c1), false);
          cm.getLastAccessStats(TARGET_CACHE_LEVEL, &miss, &newBlockBaseAddress, &oldBlockBaseAddress);
          if(miss)
            we_miss_block((long long) (originalImage->data), originalImage->size*sizeof(unsigned char), newBlockBaseAddress, oldBlockBaseAddress, blockSize, MOTION_IMAGE);

//          marker+=(originalImage->data[r1*originalImage->cols+c1]==FG)? 1:0;
          marker+=(originalImage->data[r1*originalImage->cols+c1]==BG)? 0:1;
        }
      }
      cm.access((long long) (outputImage->data+r*originalImage->cols+c), false);
      cm.getLastAccessStats(TARGET_CACHE_LEVEL, &miss, &newBlockBaseAddress, &oldBlockBaseAddress);
      if(miss)
        we_miss_block((long long) (outputImage->data), outputImage->size*sizeof(unsigned char), newBlockBaseAddress, oldBlockBaseAddress, blockSize, MAX_QL); // EROSION_IMAGE);

      if((float)marker/count>ER_THRESHOLD){
        //outputImage->data[r*originalImage->cols+c]=FG;
        we_assign_uchar(outputImage->data + r*originalImage->cols+c, FG, EROSION_IMAGE);
      } else {
        //outputImage->data[r*originalImage->cols+c]=BG;
        we_assign_uchar(outputImage->data + r*originalImage->cols+c, BG, EROSION_IMAGE);
      }
    }
  }  
}

/*
 * Perfroms an overlapping of the features on an original image. the featuresImage must have 1 channel.
 */
void overlap_features_filter(sImage* originalImage, sImage* featuresImage, int threshold, sImage* outputImage){
  for(int r=0; r < outputImage->rows; r++)
    for(int c=0; c < outputImage->cols; c++){
      if(featuresImage->data[r*outputImage->cols+c]>threshold){
        outputImage->data[(r*outputImage->cols+c)*outputImage->depth] = 0;
        outputImage->data[(r*outputImage->cols+c)*outputImage->depth+1] = 0;
        outputImage->data[(r*outputImage->cols+c)*outputImage->depth+2] = 255;
      } else{
        for(int d=0; d < outputImage->depth; d++)
          outputImage->data[(r*outputImage->cols+c)*outputImage->depth+d] = 
                          originalImage->data[(r*outputImage->cols+c)*outputImage->depth+d]; 
      }
    }
}

/*
 * Identifies blobs in a black/white map and returns the map with the bounding box (by means of the second
 * parameter), the nuber of identified bounding boxes (third parameter) and the list of bounding boxs' coordinates
 * (by means of the return statement). The caller has to free dynamically allocated memory.
 */
int** blob_detection(sImage* originalImage, sImage* blobImage, int* num_of_bbs, bool output) {
  int r,c, i, count;
  int **bb_coords;

  //do consider that 0 means no blob. >0 is a blob. in this way there is an extra identified bb in position [0]
  //that is later discarded during the print on the screen and in the modification of the application
  for(r=0, count=1; r<blobImage->rows; r++){
    for (c=0; c<blobImage->cols; c++){
      cm.access((long long) (originalImage->data+r*blobImage->cols+c), false);    
      cm.getLastAccessStats(TARGET_CACHE_LEVEL, &miss, &newBlockBaseAddress, &oldBlockBaseAddress);
      if(miss)
        we_miss_block((long long) (originalImage->data), originalImage->size*sizeof(unsigned char), newBlockBaseAddress, oldBlockBaseAddress, blockSize, EROSION_IMAGE);

      int value;
      if(originalImage->data[r*blobImage->cols+c] == FG) {
        if(r>0 && c>0){
          cm.access((long long) (blobImage->data+(r-1)*blobImage->cols+(c-1)), false);    
          cm.getLastAccessStats(TARGET_CACHE_LEVEL, &miss, &newBlockBaseAddress, &oldBlockBaseAddress);
          if(miss)
            we_miss_block((long long) (blobImage->data), blobImage->size*sizeof(unsigned char), newBlockBaseAddress, oldBlockBaseAddress, blockSize, BLOB_IMAGE);
        }
        if(r>0){
          cm.access((long long) (blobImage->data+(r-1)*blobImage->cols+(c)), false);    
          cm.getLastAccessStats(TARGET_CACHE_LEVEL, &miss, &newBlockBaseAddress, &oldBlockBaseAddress);
          if(miss)
            we_miss_block((long long) (blobImage->data), blobImage->size*sizeof(unsigned char), newBlockBaseAddress, oldBlockBaseAddress, blockSize, BLOB_IMAGE);

          cm.access((long long) (blobImage->data+(r-1)*blobImage->cols+(c+1)), false);    
          cm.getLastAccessStats(TARGET_CACHE_LEVEL, &miss, &newBlockBaseAddress, &oldBlockBaseAddress);
          if(miss)
            we_miss_block((long long) (blobImage->data), blobImage->size*sizeof(unsigned char), newBlockBaseAddress, oldBlockBaseAddress, blockSize, BLOB_IMAGE);
        }
        if(c>0){
          cm.access((long long) (blobImage->data+(r)*blobImage->cols+(c-1)), false);    
          cm.getLastAccessStats(TARGET_CACHE_LEVEL, &miss, &newBlockBaseAddress, &oldBlockBaseAddress);
          if(miss)
            we_miss_block((long long) (blobImage->data), blobImage->size*sizeof(unsigned char), newBlockBaseAddress, oldBlockBaseAddress, blockSize, BLOB_IMAGE);
        }
        
        if(r>0 && c>0 && blobImage->data[(r-1)*blobImage->cols+(c-1)] > 0){
          value=blobImage->data[(r-1)*blobImage->cols+(c-1)];
        }else if(r>0 && blobImage->data[(r-1)*blobImage->cols+(c)] > 0){
          value=blobImage->data[(r-1)*blobImage->cols+(c)];
        }else if (r>0 && c<blobImage->cols && blobImage->data[(r-1)*blobImage->cols+(c+1)] > 0){
          value=blobImage->data[(r-1)*blobImage->cols+(c+1)];
        }else if (c>0 && blobImage->data[(r)*blobImage->cols+(c-1)] > 0){
          value=blobImage->data[(r)*blobImage->cols+(c-1)];
        }else {
          value=count;
          count++;
        }        
      } else
        value=0;

      cm.access((long long) (blobImage->data+r*blobImage->cols+c), false);    
      cm.getLastAccessStats(TARGET_CACHE_LEVEL, &miss, &newBlockBaseAddress, &oldBlockBaseAddress);
      if(miss)
        we_miss_block((long long) (blobImage->data), blobImage->size*sizeof(unsigned char), newBlockBaseAddress, oldBlockBaseAddress, blockSize, MAX_QL); // BLOB_IMAGE);

      //DO NOTE: I don't inject errors in this write since we have a large reuse of this value in subsequent loop interations (thus it should be in L1)
      //we will simulate errors later in subsequent loops.
      blobImage->data[r*blobImage->cols+c] = value;
    }
  }

  /*pay attention. numbering of blobs starts from 1 (0 is the background) so we have to shift left the index by 1*/
  count--;
  bb_coords = (int**) malloc(sizeof(int*)*(count));
  for(i=0; i<count; i++) {
    bb_coords[i] = (int*) malloc(sizeof(int)*4); //bottom_x, bottom_y, top_x, top_y
    bb_coords[i][0] = blobImage->cols -1;
    bb_coords[i][1] = blobImage->rows -1;
    bb_coords[i][2] = 0;
    bb_coords[i][3] = 0;
  }
  for(r=0; r<blobImage->rows; r++){
    for (c=0; c<blobImage->cols; c++){
      cm.access((long long) (blobImage->data+r*blobImage->cols+c), false);    
      cm.getLastAccessStats(TARGET_CACHE_LEVEL, &miss, &newBlockBaseAddress, &oldBlockBaseAddress);
      if(miss){
        we_miss_block((long long) (blobImage->data), blobImage->size*sizeof(unsigned char), newBlockBaseAddress, oldBlockBaseAddress, blockSize, BLOB_IMAGE);
      }
      char value = blobImage->data[r*blobImage->cols+c];
      if(value > 0 && value <=count) {
        if(c<bb_coords[value-1][0])
          bb_coords[value-1][0] = c;
        if(r<bb_coords[value-1][1])
          bb_coords[value-1][1] = r;
        if(c>bb_coords[value-1][2])
          bb_coords[value-1][2] = c;
        if(r>bb_coords[value-1][3])
          bb_coords[value-1][3] = r;
      }
    }
  }
  
  if(output)
    for(r=0; r<blobImage->rows; r++)
      for (c=0; c<blobImage->cols; c++){
        blobImage->data[r*blobImage->cols+c]=BG;
        for(i = 0; i < count; i++)
          if((c==bb_coords[i][0] && r>=bb_coords[i][1] && r<=bb_coords[i][3]) ||
             (c==bb_coords[i][2] && r>=bb_coords[i][1] && r<=bb_coords[i][3]) ||
             (r==bb_coords[i][1] && c>=bb_coords[i][0] && c<=bb_coords[i][2]) ||
             (r==bb_coords[i][3] && c>=bb_coords[i][0] && c<=bb_coords[i][2]) )
            blobImage->data[r*blobImage->cols+c]=FG;
      }

  *num_of_bbs = count;
  return bb_coords;
}
