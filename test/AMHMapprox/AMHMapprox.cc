#include <stdio.h>
#include "/home/amir/sniper-mem/include/sim_api.h" 

#define ToUnsignedInt(X) *((unsigned long long*)(&X))

int main()
{
	FILE * fptr;
   	fptr = fopen("./Output.txt","w");	
	printf("This is the start of test program.\n");
	long int vector[10000];
	double ber = 0.2;
	printf("Program: Start Address %lx\n",(long long int)&(vector[0]));
	printf("Program: End Address %lx\n",(long long int) (&vector[9999] + sizeof(long int) - 1));
	AMHM_approx((long long int)&(vector[0]), (long long int) (&vector[9999] + sizeof(long int) - 1));
	AMHM_qual(ToUnsignedInt(ber));
	ber = 0.5;
	AMHM_approx((long long int)&(vector[100]), (long long int) (&vector[9999] + sizeof(long int) - 1));
	AMHM_qual(ToUnsignedInt(ber));
	ber = 0;
	AMHM_approx((long long int)&(vector[1000]), (long long int) (&vector[9999] + sizeof(long int) - 1));
	AMHM_qual(ToUnsignedInt(ber));
	if(fptr == NULL)
	   {
	      printf("Error!");   
	      return 1;             
	   }
	vector[0] = 0xCCCCCCCCCCCCCCCC;
	for (int i = 0; i < 9999; i++)
			vector[i+1] = vector [i];
	for (int i = 0; i < 9999; i++)
		fprintf(fptr,"0x%llx\n",vector[i]);
	fclose(fptr);
	AMHM_accurate((long long int)&(vector[0]));
	AMHM_accurate((long long int)&(vector[1000]));
	AMHM_accurate((long long int)&(vector[100]));
	printf("This is the end of test program.\n");
	return 0;
}
