#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

//TODO add stats logging ?

#define DELIMITS "-"

/*these macro and global constant describe the various write error
probabilities for the selected QLs*/
//#define N_QLS 2
//const double EPs[N_QLS] = {0.3, 1};
#define N_QLS 9
const double EPs[N_QLS] = {10E-9, 10E-8, 10E-7, 10E-6, 10E-5, 10E-4, 10E-3, 10E-2, 10E-1};

/* ql_config is an array where each index specify a step in the 
application and the corresponding value the assigned QL. num_ql_configs is
the number of configurable steps. Do note that such an information has to be
acquired a the beginning of teh execution.
*/
#define MAX_QL_STEPS 10
int ql_config[MAX_QL_STEPS];
int num_ql_configs;

/* This array is used to simulate the content of the cache when the first miss on a block occurs.
Two strategies are implemented: 1) all bytes are set to random values or 2) o zero values
*/
#define BLOCKMAXSIZE 2048
char dummyCacheBlock[BLOCKMAXSIZE];
bool randomDummyCacheBlockInit;

/*Statistics*/
long long count1to0, count0to1, countnofault;

void we_setup(char* , int);
long long we_get_dummy_cache_baseaddress(long long);
void we_assign(unsigned char*, unsigned char*, unsigned char*, size_t, int);
void we_miss_block(long long, long long, long long, long long, long long, int);
void we_assign_int(int*, int, int);
void we_assign_float(float*, float, int);
void we_assign_double(double*, double, int);
void we_assign_char(char*, char, int);
int we_activate(int);


/*
 * Setup the environment (srand and QL configuration to be used for the various
 * application steps.
 */
void we_setup(char* ql_config_str, int seed, bool randomCacheInit){
  char *token;
  if(seed)
    srand(seed);
  else
    srand(time(NULL));

  /*parse ql configuration*/  
  num_ql_configs = 0;
  token = strtok(ql_config_str, DELIMITS);   
  while(token != NULL) {
    ql_config[num_ql_configs] = atoi(token);
    if(ql_config[num_ql_configs]>=N_QLS || ql_config[num_ql_configs]<0){
      fprintf(stderr, "Not defined QL level %d\n", ql_config[num_ql_configs]);
      exit(1);
    }
    num_ql_configs++;
    if(num_ql_configs > MAX_QL_STEPS){
      fprintf(stderr, "Update MAX_QL_STEPS macro to a larger value\n");
      exit(1);
    }
    token = strtok(NULL, DELIMITS);
  }
  //I fill the rest of the vector with the maximum approx. I need this trick to tune when I need max approx
  for(int i=num_ql_configs; i<MAX_QL_STEPS;i++)
    ql_config[i]= N_QLS-1;
#ifdef DEBUGP
  printf("num_ql_configs: %d\n", num_ql_configs);
  for(int i=0; i<MAX_QL_STEPS /*num_ql_configs*/; i++)
    printf("%d - %d \n", i, ql_config[i]);
#endif
  
  if(randomCacheInit) {
    for(int i=0; i<BLOCKMAXSIZE; i++)
      dummyCacheBlock[i]=rand();
  } else {
    for(int i=0; i<BLOCKMAXSIZE; i++)
      dummyCacheBlock[i]=0;  
  }
  randomDummyCacheBlockInit = randomCacheInit;
  
  /*reset stats*/
  count1to0 = count0to1 = countnofault = 0;
}

long long we_get_dummy_cache_baseaddress(long long blockSize) {
  if(randomDummyCacheBlockInit) {
    return (long long) dummyCacheBlock + rand()%(BLOCKMAXSIZE-blockSize);
  } else{
    return (long long) dummyCacheBlock;
  }
}


/*
 * This function returns true if the write error for a given QL
 * occurs for the current bit-wise write.
 */
int we_activate(int currQL) {
  double r = rand() / ((double) RAND_MAX);
//#ifdef DEBUGP
//  printf("Prob: %lf, %lf\n", r, EPs[currQL]);
//#endif
  return EPs[currQL] >= r;
}

/*
 * This function performs a write in the memory simulating the possible write errors.
 * Required parameters are the old and the new value to be assigned and the pointer
 * to the variable where to store the actually written value (containing possible 
 * errors. We use pointers to char to be generic and work with any type of data.
 * Thus it is necessary to receive also the size of the actual data type. Finally
 * write errors are activated based on a specified QL
 */
void we_assign(unsigned char* oldValue, unsigned char* newValue, unsigned char* writtenValue, size_t size, int currConfig){
  unsigned char mask, newByte, oldBit, newBit, writtenByte;
  int i, j; /*i scans bytes, j bits in byte*/

#ifdef DEBUGP
  printf("old value: ");
  for(i=0;i<size;i++)
    printf("%d ", *(oldValue+i));
  printf("\nnew value: ");
  for(i=0;i<size;i++)
    printf("%d ", *(newValue+i));
  printf("\n");
  printf("Curr step: %d curr QL: %d\n", currConfig, ql_config[currConfig]);
#endif

  for(i=0; i<size; i++){
    writtenByte = 0;
    for(j=0, mask=1; j<8; j++, mask=mask<<1){
      oldBit = *(oldValue+i) & mask;
      newByte = *(newValue+i);
      newBit = newByte & mask;
//      printf("bits: %d %d\n", oldBit, newBit);
      if(oldBit>newBit && we_activate(ql_config[currConfig])){ /*1 -> 0 transition*/
        writtenByte = (writtenByte & ~mask) | (mask); /*write 1 in the bit*/
//        printf("1->0 %d %d: %d %d %d %d\n", i, j, mask, writtenByte, (writtenByte & ~mask), writtenByte);
        count1to0++;
      } else if (oldBit<newBit && we_activate(ql_config[currConfig])){ /*0 -> 1 transition*/
        writtenByte = (writtenByte & ~mask); /*write 0 in the bit*/
//        printf("0->1 %d %d: %d %d %d %d\n", i, j, mask, writtenByte, (writtenByte & ~mask), writtenByte);
        count0to1++;
      } else {
        writtenByte = (writtenByte & ~mask) | (newByte & mask);
//        printf("%d %d: %d %d %d %d %d %d\n", i, j, mask, writtenByte, newByte, (writtenByte & ~mask), (newByte & mask), writtenByte);
        countnofault++;
      }
    }
    *(writtenValue+i) = writtenByte;
  }

#ifdef DEBUGP
  printf("written value: ");
  for(i=0;i<size;i++)
    printf("%d ", *(writtenValue+i));
  printf("\n");
#endif

}

/*
 * This functions simulates the write errors when a block is replaced in the cache due to a cache miss. Required parameters are:
 * - arrayBaseAddr: the base address of the array that is "loaded" in the cache 
 * - arraySizeByte: the dimension of the array IN BYTE
 * - blockBaseAddr: the base address of the block that is "loaded" in the cache
 * - oldBlockBaseAddr: the base address of the old block that is replaced in the cache DO NOTE: think if it is correct!
 * - blockSize: the size in byte of the block
 * - currConfig: the number of the current step in the application pipeline (to get the associated QL)
 */
void we_miss_block(long long arrayBaseAddr, long long arraySizeByte, long long blockBaseAddr, long long oldBlockBaseAddr, long long blockSize, int currConfig) {
  //if the content of the block is invalid, we use a dummy array as a previous content of the cache
  if(oldBlockBaseAddr == -1)
    oldBlockBaseAddr = we_get_dummy_cache_baseaddress(blockSize);
  //the following two if statement are used to avoid segfaults when simulating errors in blocks partially storing the array variable (first block and last block)
  //in fact if the array is not aligned to blocks in the memory, simulating write errors in the first and last block may cause overflows/underflows in write operations...
  //the solution has been to limit the error injection to the only content of the array
  //DO NOTE: 2 possible alternative strategies may be to force the two blocks to be QL0 or to translate addresses to simulate the array to be aligned in the memory 
  //(this last one does not solve the problem with the last block if the size of the array is not multiple of the size of the block)
  if(arrayBaseAddr > blockBaseAddr){
    long long toBeRemoved = arrayBaseAddr - blockBaseAddr;
    blockSize = blockSize - toBeRemoved;    
    //std::cout << "-X "  << std::hex << blockSize << " " << arrayBaseAddr << " " << blockBaseAddr << " "; 
    blockBaseAddr = arrayBaseAddr;
    //std::cout << blockBaseAddr << " " << oldBlockBaseAddr << " ";
    oldBlockBaseAddr = oldBlockBaseAddr + toBeRemoved; //rightshift of the base address f the block of the old array of the same amount of bytes
    //std::cout << oldBlockBaseAddr << std::dec  << std::endl;
  }
  //std::cout << "Z " << std::hex << blockBaseAddr << " " << blockSize << " +" << (blockBaseAddr + blockSize) << " " << arrayBaseAddr << " " << arraySizeByte << " +" << (arrayBaseAddr + arraySizeByte) << std::dec << std::endl;
  if(blockBaseAddr + blockSize > arrayBaseAddr + arraySizeByte){ //both if statements are true ONLY if the array is not aligned on the block and it smaller than a block
    blockSize = arrayBaseAddr + arraySizeByte - blockBaseAddr;
    //std::cout << "-> " << std::hex << blockSize << std::dec << std::endl; 
  }
  we_assign((unsigned char*) oldBlockBaseAddr, (unsigned char*) blockBaseAddr, (unsigned char*) blockBaseAddr, blockSize, currConfig);
}        

/*
 * Following functions simulate write errors while assigning a new value to a variable. Actually this assumes a write through policy but if a variable is not assigned twice
 * in a row or written and immeditely read without a cache miss of that block in between, the behavior is the same of the write back. The conclusion is that the output array CANNOT be used as an accumulator.
 * write through policy is much more susceptible to write errors than write back.
 */

void we_assign_int(int *dest, int source, int currConfig){
  int oldValue = *dest;
#ifdef DEBUGP
  printf("old value: %d\n", *dest);
  printf("to save: %d\n", source);
#endif

  we_assign((unsigned char*) &oldValue, (unsigned char*) &source, (unsigned char*) dest, sizeof(int), currConfig);

#ifdef DEBUGP
  printf("\nstats:\nnoerror: %lld\n1->0: %lld\n0->1: %lld\n\n", countnofault, count1to0, count0to1);
  printf("to save: %d\n", source);
#endif
}

void we_assign_float(float *dest, float source, int currConfig){
  float oldValue = *dest;
  we_assign((unsigned char*) &oldValue, (unsigned char*) &source, (unsigned char*) dest, sizeof(float), currConfig);
}

void we_assign_double(double *dest, double source, int currConfig){
  double oldValue = *dest;
  we_assign((unsigned char*) &oldValue, (unsigned char*) &source, (unsigned char*) dest, sizeof(double), currConfig);
}

void we_assign_char(char *dest, char source, int currConfig){
  char oldValue = *dest;
  we_assign((unsigned char*) &oldValue, (unsigned char*) &source, (unsigned char*) dest, sizeof(char), currConfig);
}

void we_assign_uchar(unsigned char *dest, unsigned char source, int currConfig){
  unsigned char oldValue = *dest;
  we_assign((unsigned char*) &oldValue, (unsigned char*) &source, (unsigned char*) dest, sizeof(char), currConfig);
}

