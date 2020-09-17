#include "../../include/sim_api.h"

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include <stdint.h>
#include <stdio.h>
#include "../../common/core/memory_subsystem/mem_component.h"

#define ToUnsignedInt(X) *((unsigned long long*)(&X))
#define ARRAY_SIZE 10000

double ber=0.0005; // FI Uniform 0.01 sometimes crash!

int main(int argc,char **argv)
{
	FILE * fptr1;
	//FILE * fptr2;
	int i = 0;
	int tmp_val = 0;
   	fptr1 = fopen("Output.txt","w");
   	//fptr2 = fopen("Output2.txt","w");
	printf("This is the start of test program.\n");
	int vector[ARRAY_SIZE];
	printf("Program: Vector Start Address %llx\n",(uint64_t)&(vector[0]));
	printf("Program: Vector End Address %llx\n",(uint64_t) (&vector[9999] + sizeof(int) - 1));
	if(fptr1 == NULL)
	   {
	      printf("Error!");   
	      return 1;             
	   }
	vector[0] = 0xCCCCCCCC;
	for (i = 0; i < ARRAY_SIZE-1; i++)
			vector[i+1] = vector[0];
	AMHM_approx((long long int)&(vector[0]),(long long int)&(vector[0])+ (ARRAY_SIZE * sizeof(int)));
	AMHM_qual(ToUnsignedInt(ber));
	//fwrite(vector, 2, ARRAY_SIZE, fptr);
	for (i = 0; i < ARRAY_SIZE; i++){
		fprintf(fptr1, "0x%llx\n", vector[i]);
		//fprintf(fptr2, "0x%llx\n", vector[i]);
	}
	/*for (i = 0; i < ARRAY_SIZE-1; i++){
		vector[2] = vector[i];
		//fprintf(fptr2, "0x%llx\n", vector[i]);
	}*/


	fclose(fptr1);
	//fclose(fptr2);
	AMHM_accurate((long long int)&(vector[0]));
	printf("This is the end of test program.\n");
	return 0;
}
