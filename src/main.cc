/*******************************************************
                          main.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
#include "cache.h"
#include "cache.cc"
#include<stdio.h>


using namespace std;
int main(int argc, char *argv[])
{
	
	ifstream fin;
	//FILE * pFile;
	int i;
	if(argv[1] == NULL){
		 printf("input format: ");
		 printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
		 exit(0);
        }

	/*****uncomment the next five lines*****/
	int cache_size = atoi(argv[1]);
	int cache_assoc= atoi(argv[2]);
	int blk_size   = atoi(argv[3]);
	int num_processors = atoi(argv[4]);/*1, 2, 4, 8*/
	int protocol   = atoi(argv[5]);	 /*0:MSI, 1:MESI, 2:Dragon*/
	char *fname =  (char *)malloc(20);
 	fname = argv[6];

	fin.open(fname);
	//****************************************************//
	//**printf("===== Simulator configuration =====\n");**//
	//*******print out simulator configuration here*******//
	//****************************************************//
		//*********************************************//
        //*****create an array of caches here**********//
	//*********************************************//	
	//pFile = fopen (fname,"r");
	Cache **cachesArray = new Cache *[num_processors];
	for(i=0;i<num_processors;i++)
		cachesArray[i]= new Cache(cache_size,cache_assoc,blk_size,num_processors);
	
	if(!fin.is_open())
	{   
		printf("Trace file problem\n");
		exit(0);
	}
	///******************************************************************//
	//**read trace file,line by line,each(processor#,operation,address)**//
	//*****propagate each request down through memory hierarchy**********//
	//*****by calling cachesArray[processor#]->Access(...)***************//
	//******************************************************************//
	//fclose(pFile);
	ulong address, processor; 
	char op;
	while(!fin.eof())
	{
		fin>>processor>>op>>hex>>address;
		//cout<<"\n heyo"<<" \t"<<address<<"\t"<<processor<<"\t"<<op;
		//processor=processor-48;
		
		
		if(protocol == 0)
			cachesArray[processor]->MSI(processor,address,op,cachesArray);
		else if(protocol == 1)
			cachesArray[processor]->MESI(processor,address,op,cachesArray);
		else if(protocol == 2)
			cachesArray[processor]->dragon(processor,address,op,cachesArray);
		
		
	} 
	//********************************//
	//print out all caches' statistics //
	//********************************//
	printf("===== 506 Personal infromation =====");
		printf("\nFirst Name: Kashyap  Last Name: Ravichandran");
		printf("\nUnity ID: kravich2");
		printf("\nECE 492 Students? NO");
		printf("\n===== 506 SMP Simulator configuration =====");
		printf("\nL1_SIZE: %d\t",cache_size);	
		printf("\nL1_ASSOC: %d\t",cache_assoc);
		printf("\nL1_BLOCKSIZE: %d\t", blk_size);
		printf("\nNUMBER OF PROCESSORS: %d\t", num_processors);
		printf("\nCoherence Protocol: \t");
		if(protocol==0)
		printf("MSI");
		else if(protocol ==1)
		printf("MESI");
		else if(protocol==2)
		printf("Dragon");
		cout<<"\nTRACE FILE: \t"<<fname<<"\n";
		
		for(i=0;i<num_processors;i++)
		{
			cachesArray[i]->printStats(i,protocol);
			cout<<endl;
		}
}
