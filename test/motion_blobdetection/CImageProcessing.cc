#include "image_processing.h"

//#include <omp.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

//image twisting macros
#define PI 3.1415265359
#define DRAD (180.0f/PI)

//motion detection macros
#define MD_THRESHOLD 10
#define EROSION_MAP_SIZE 7 /*must be an odd number*/
#define FG 0
#define BG 255
#define ER_THRESHOLD 0.7

/*
 * Performs the edge detection according to the Sobel algorithm
 */
void sobel_edge_detection(sImage* greyImage, sImage* edgeImage){
  int  GX[3][3];
  int  GY[3][3];
    
  unsigned int X, Y;
  int  I, J;
  long sumX, sumY;
  int  SUM;
    
  /* 3x3 GX Sobel mask.  Ref: www.cee.hw.ac.uk/hipr/html/sobel.html */
  GX[0][0] = -1; GX[0][1] = 0; GX[0][2] = 1;
  GX[1][0] = -2; GX[1][1] = 0; GX[1][2] = 2;
  GX[2][0] = -1; GX[2][1] = 0; GX[2][2] = 1;
    
  /* 3x3 GY Sobel mask.  Ref: www.cee.hw.ac.uk/hipr/html/sobel.html */
  GY[0][0] =  1; GY[0][1] =  2; GY[0][2] =  1;
  GY[1][0] =  0; GY[1][1] =  0; GY[1][2] =  0;
  GY[2][0] = -1; GY[2][1] = -2; GY[2][2] = -1;
    
  /*---------------------------------------------------
    SOBEL ALGORITHM STARTS HERE
    ---------------------------------------------------*/
  //#pragma omp parallel for private(sumX, sumY, SUM, I, J, X, Y) //default (shared) // shared(edgeImage, GX, GY, greyImage) //collapse(2)
  for(Y=0; Y<=(greyImage->rows-1); Y++)  {
    for(X=0; X<=(greyImage->cols-1); X++)  {
      sumX = 0;
      sumY = 0;
            
      /* image boundaries */
      if(Y==0 || Y==greyImage->rows-1 || X==0 || X==greyImage->cols-1)
        SUM = 0;
      /* Convolution starts here */
      else   {              
        /*-------X GRADIENT APPROXIMATION------*/
        for(I=-1; I<=1; I++)  {
          for(J=-1; J<=1; J++)  {
            sumX = sumX + (int)( (*(greyImage->data + X + I + (Y + J)*greyImage->cols)) * GX[I+1][J+1]);
          }
        }
        //if(sumX>255) sumX=255;
        //else if(sumX<0) sumX=0;
             
        /*-------Y GRADIENT APPROXIMATION-------*/
        for(I=-1; I<=1; I++)  {
          for(J=-1; J<=1; J++)  {
            sumY = sumY + (int)( (*(greyImage->data + X + I + (Y + J)*greyImage->cols)) * GY[I+1][J+1]);
          }
        }
        //if(sumY>255) sumY=255;
        //else if(sumY<0) sumY=0;
                
        //SUM = abs(sumX) + abs(sumY); /*---GRADIENT MAGNITUDE APPROXIMATION (Myler p.218)----*/
        SUM= sqrt(sumX*sumX+sumY*sumY);
      }        
      *(edgeImage->data + X + Y*greyImage->cols) = /*255 -*/ (unsigned char)(SUM);
    }
  }
}

/*
 * Performs the image downsampling by using the bilinear interpolation
 */
void image_downsample(sImage* image, sImage* outputImage, double factor){
  //http://en.wikipedia.org/wiki/Bilinear_interpolation
  int r, c; //current coordinates
  int olc, ohc, olr, ohr; //coordinates of the original image used for bilinear interpolation
  int index; //linearized index of the point  
  unsigned char q11, q12, q21, q22;
  float accurate_c, accurate_r; //the exact scaled point
  int k;
  
  //#pragma omp parallel for private(r, c, olc, ohc, olr, ohr, index, q11, q12, q21, q22, k, accurate_c, accurate_r) //default(shared) //shared(edgeImage, GX, GY, greyImage) //collapse(2)
  for(r=0; r<outputImage->rows; r++){
    for(c=0; c<outputImage->cols; c++){
      accurate_c = (float)c*factor;
      olc=accurate_c;
      ohc=olc+1;
      if(!(ohc<image->cols))
        ohc=olc;

      accurate_r = (float)r*factor;
      olr=accurate_r;
      ohr=olr+1;
      if(!(ohr<image->cols))
        ohr=olr;
             
      index= (c + r*outputImage->cols)*outputImage->depth; //outputImage->depth bytes per pixel
      for(k=0; k<outputImage->depth; k++){
        q11=image->data[(olc + olr*image->cols)*outputImage->depth+k];
        q12=image->data[(olc + ohr*image->cols)*outputImage->depth+k];
        q21=image->data[(ohc + olr*image->cols)*outputImage->depth+k];
        q22=image->data[(ohc + ohr*image->cols)*outputImage->depth+k];
        outputImage->data[index+k] = (unsigned char) (q11*(ohc-accurate_c)*(ohr-accurate_r) + 
                                                     q21*(accurate_c-olc)*(ohr-accurate_r) + 
                                                     q12*(ohc-accurate_c)*(accurate_r-olr) +
                                                     q22*(accurate_c-olc)*(accurate_r-olr));
      }
    }  
  }
}

/*
 * Performs an image twisting
 */ 
void image_twisting(sImage* image, sImage* outputImage, float factor){
  int r, c; //current coordinates
  int cr, cc; //coordinates of the center of the image
  float x, y, radius, theta; //cartesian coordinates and polar ones of the twisted point (x=column number, y=row number)
  int index; //linearized index of the point
  float distortion_gain = 1000.0 * factor;
  
  //http://en.wikipedia.org/wiki/Bilinear_interpolation
  int lx, ly, hx, hy; //4 coordinates for bilinear interpolation
  unsigned char q11, q12, q21, q22;
  int k;
     
  cr=image->rows/2;
  cc=image->cols/2;

  //#pragma omp parallel for private (r, c, theta, radius, x, y, index, lx, hx, ly, hy, q11, q12, q21, q22, k) //default (shared)
  for(r=0; r<image->rows; r++){
    for(c=0; c<image->cols; c++){
      radius = sqrt((c - cc) * (c - cc) + (r - cr) * (r - cr));
      theta = radius/DRAD/image->rows*distortion_gain; //radius/2*DRAD;
      
      x = cos(theta) * (c - cc) + sin(theta) * (r - cr) + cc;
      y = -sin(theta) * (c - cc) + cos(theta) * (r - cr) + cr;
      index= (c + r*image->cols)*outputImage->depth; //outputImage->depth bytes per pixel
      if(x>=0 && y>=0 && y<image->rows-1 && x<image->cols-1) {
        //bilinear interpolation
        lx=x;
        hx=x+1;
        ly=y;
        hy=y+1;
        
        for(k=0; k<outputImage->depth; k++){
          q11=image->data[(lx + ly*image->cols)*outputImage->depth+k];
          q12=image->data[(lx + hy*image->cols)*outputImage->depth+k];
          q21=image->data[(hx + ly*image->cols)*outputImage->depth+k];
          q22=image->data[(hx + hy*image->cols)*outputImage->depth+k];
          outputImage->data[index+k]= (unsigned char) (q11*(hx-x)*(hy-y) + 
                                                     q21*(x-lx)*(hy-y) + 
                                                     q12*(hx-x)*(y-ly) +
                                                     q22*(x-lx)*(y-ly));
        }         
      } else{
        for(k=0; k<outputImage->depth; k++){
          outputImage->data[index+k] = 0;
        }      
      }
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
      outputImage->data[r*originalImage->cols+c]=(abs(originalImage->data[r*originalImage->cols+c]-backgroundImage->data[r*originalImage->cols+c])>MD_THRESHOLD)?FG:BG;
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
          marker+=(originalImage->data[r1*originalImage->cols+c1]==FG)? 1:0;
        }
      }
      if((float)marker/count>ER_THRESHOLD){
        outputImage->data[r*originalImage->cols+c]=FG;
      } else {
        outputImage->data[r*originalImage->cols+c]=BG;                
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
 * Identifies blobs in a black/white map and returns the map with the bounding box.
 */
void blob_detection(sImage* originalImage, sImage* blobImage) {
  int r,c, i, count;
  int **bb_coords;
  for(r=0, count=1; r<blobImage->rows; r++){
    for (c=0; c<blobImage->cols; c++){
      if(originalImage->data[r*blobImage->cols+c] == FG) {
        if(r>0 && c>0 && blobImage->data[(r-1)*blobImage->cols+(c-1)] > 0)
          blobImage->data[r*blobImage->cols+c]=blobImage->data[(r-1)*blobImage->cols+(c-1)];
        else if(r>0 && blobImage->data[(r-1)*blobImage->cols+(c)] > 0)
          blobImage->data[r*blobImage->cols+c]=blobImage->data[(r-1)*blobImage->cols+(c)];
        else if (r>0 && c<blobImage->cols && blobImage->data[(r-1)*blobImage->cols+(c+1)] > 0)
          blobImage->data[r*blobImage->cols+c]=blobImage->data[(r-1)*blobImage->cols+(c+1)];
        else if (c>0 && blobImage->data[(r)*blobImage->cols+(c-1)] > 0)
          blobImage->data[r*blobImage->cols+c]=blobImage->data[(r)*blobImage->cols+(c-1)];
        else {
          blobImage->data[r*blobImage->cols+c]=count;
          count++;
        }        
      } else
        blobImage->data[r*blobImage->cols+c]=0;
    }
  }
  printf("count %d\n", count);

  if(count >0){
    bb_coords = (int**) malloc(sizeof(int*)*count);
    for(i=0; i<count; i++) {
      bb_coords[i] = (int*) malloc(sizeof(int)*4); //top_x, top_y, bottom_x, bottom_y
      bb_coords[i][0] = blobImage->rows -1;
      bb_coords[i][1] = blobImage->cols -1;
      bb_coords[i][2] = 0;
      bb_coords[i][3] = 0;
    }
    for(r=0; r<blobImage->rows; r++){
      for (c=0; c<blobImage->cols; c++){
        if(blobImage->data[r*blobImage->cols+c] >0) {
          if(r<bb_coords[blobImage->data[r*blobImage->cols+c]][0])
            bb_coords[blobImage->data[r*blobImage->cols+c]][0] = r;
          if(c<bb_coords[blobImage->data[r*blobImage->cols+c]][1])
            bb_coords[blobImage->data[r*blobImage->cols+c]][1] = c;
          if(r>bb_coords[blobImage->data[r*blobImage->cols+c]][2])
            bb_coords[blobImage->data[r*blobImage->cols+c]][2] = r;
          if(c>bb_coords[blobImage->data[r*blobImage->cols+c]][3])
            bb_coords[blobImage->data[r*blobImage->cols+c]][3] = c;
        }
      }
    }
    for(i=0; i<count; i++)
      printf("%d %d -> %d %d\n", bb_coords[i][0], bb_coords[i][1], bb_coords[i][2], bb_coords[i][3]);


    for(r=0; r<blobImage->rows; r++)
      for (c=0; c<blobImage->cols; c++)
        blobImage->data[r*blobImage->cols+c]=FG;

    for(r=0; r<blobImage->rows; r++)
      for (c=0; c<blobImage->cols; c++)
        for(i = 1; i < count; i++)
          if(r==bb_coords[i][0] && c>=bb_coords[i][1] && c<=bb_coords[i][3] || 
             r==bb_coords[i][2] && c>=bb_coords[i][1] && c<=bb_coords[i][3] || 
             c==bb_coords[i][1] && r>=bb_coords[i][0] && r<=bb_coords[i][2] || 
             c==bb_coords[i][3] && r>=bb_coords[i][0] && r<=bb_coords[i][2] )
            blobImage->data[r*blobImage->cols+c]=BG;
    for(i=0; i<count; i++)
      free(bb_coords[i]);
    free(bb_coords);
  }

}


//TODO not use it since it generate the filter matrix each time it is invoked. it makes no sense!
/*
 * Performs the gaussian filter
 */
void gaussian_filter(sImage* originalImage, sImage* finalImage, float sigma){    
  int r, c;
  int  I, J, color;
  long sumX, sumY;
  float  SUM;

  //generate the filter mask
  int maskSize = (int)ceil(3.0f*sigma);
  float * mask = new float[(maskSize * 2 + 1)*(maskSize * 2 + 1)];
  float sum = 0.0f;
  for (int a = -maskSize; a < maskSize + 1; a++) {
    for (int b = -maskSize; b < maskSize + 1; b++) {
      float temp = exp(-((float)(a*a + b*b) / (2 * sigma*sigma)));
        sum += temp;
        mask[a + maskSize + (b + maskSize)*(maskSize * 2 + 1)] = temp;
      }
  }
  // Normalize the mask
  for (int i = 0; i < (maskSize * 2 + 1)*(maskSize * 2 + 1); i++)
    mask[i] = mask[i] / sum;

  //#pragma omp parallel for private(r, c, color, sum, I, J) //shared(finalImage, originalImage, mask) //collapse(2)
  for(r=0; r<=(originalImage->rows-1); r++)  {
    for(c=0; c<=(originalImage->cols-1); c++)  {
      for(color=0; color<originalImage->depth; color++) {
        sum=0;
        /* image boundaries */
        if(c <= maskSize || c>=originalImage->cols-maskSize || r<=maskSize || r>=originalImage->rows-maskSize)
          sum = (float) originalImage->data[(c+r*originalImage->cols)*originalImage->depth + color];
        else {/* Convolution starts here */
          for(I=-maskSize; I<=maskSize; I++)  {
            for(J=-maskSize; J<=maskSize; J++)  {
              sum = sum + (float) originalImage->data[((c+I) + (r+J)*originalImage->cols)*originalImage->depth + color] 
                        * mask[I+maskSize+(J+maskSize)*(maskSize*2+1)];              
            }
          }
        }
        finalImage->data[(c+r*originalImage->cols)*originalImage->depth + color] = (unsigned char)(sum);
      }
    }
  }
  
  delete [] mask;
}

