#include "sim_api.h"

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include <stdint.h>
#include <stdio.h>
#include "../../common/core/memory_subsystem/mem_component.h"

#define ToUnsignedInt(X) *((unsigned long long*)(&X))
#define ARRAY_SIZE 10000

int main(int argc,char **argv)
{
	FILE * fptr;
	double ber = 0;
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
	ber = 1;
	add_approx((uint64_t)&vector[0],(uint64_t)&vector[ARRAY_SIZE-1]);
	set_write_er(MemComponent::DRAM, ToUnsignedInt(ber));
	ber = 1;
	set_read_er(MemComponent::DRAM, ToUnsignedInt(ber));
	for (i = 0; i < ARRAY_SIZE; i++)
		fprintf(fptr,"0x%x\n",vector[i]);
	fclose(fptr);
	remove_approx((uint64_t)&vector[0],(uint64_t)&vector[ARRAY_SIZE-1]);
	printf("This is the end of test program.\n");
}
