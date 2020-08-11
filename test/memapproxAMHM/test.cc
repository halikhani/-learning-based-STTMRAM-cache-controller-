#include "../../include/sim_api.h"

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include <stdint.h>
#include <stdio.h>
#include "../../common/core/memory_subsystem/mem_component.h"

#define ToUnsignedInt(X) *((unsigned long long*)(&X))
#define ARRAY_SIZE 10000

double ber=0.01; // FI Uniform 0.01 sometimes crash!

int main(int argc,char **argv)
{
	FILE * fptr;
	int i = 0;
   	fptr = fopen("Output.txt","w");	
	printf("This is the start of test program.\n");
	int vector[ARRAY_SIZE];
	printf("Program: Vector Start Address %x\n",(uint64_t)&(vector[0]));
	printf("Program: Vector End Address %x\n",(uint64_t) (&vector[9999] + sizeof(int) - 1));
	if(fptr == NULL)
	   {
	      printf("Error!");   
	      return 1;             
	   }
	vector[0] = 0xCCCCCCCC;
	for (i = 0; i < ARRAY_SIZE; i++)
			vector[i+1] = vector [i];
	AMHM_approx((long long int)&(vector[0]),(long long int)&(vector[0])+ (ARRAY_SIZE * sizeof(int)));
	AMHM_qual(ToUnsignedInt(ber));
	for (i = 0; i < ARRAY_SIZE; i++)
		fprintf(fptr,"0x%x\n",vector[i]);
	fclose(fptr);
	AMHM_accurate((long long int)&(vector[0]));
	printf("This is the end of test program.\n");
	return 0;
}
