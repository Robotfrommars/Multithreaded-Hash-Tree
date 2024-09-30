#include <stdio.h>     
#include <stdlib.h>   
#include <stdint.h>  
#include <inttypes.h>  
#include <errno.h>     // for EINTR
#include <fcntl.h>     
#include <unistd.h>    
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "common.h"
#include "common_threads.h"
#include <string.h>


// Print out the usage of the program and exit.
void Usage(char*);
uint32_t jenkins_one_at_a_time_hash(const uint8_t* , uint64_t );
void* tree(void*);
// block size
#define BSIZE 4096
uint64_t threads; //inputed number of threads
uint64_t blocksper; //used to calc to read
uint8_t* arr;
uint64_t bytesLeft;
uint32_t fd;
int 
main(int argc, char** argv) 
{
  
  uint32_t nblocks;

  // input checking 
  if (argc != 3)
    Usage(argv[0]);

  // open input file
  fd = open(argv[1], O_RDWR);
  if (fd == -1) {
    perror("open failed");
    exit(EXIT_FAILURE);
  }
  // use fstat to get file size
  // calculate nblocks 
	struct stat buf;
	fstat(fd,&buf);
	//calculate important values
	off_t size=buf.st_size;
	bytesLeft=size;
	//store threads to be global for use in recursive function
	threads=atoi(argv[2]);
	nblocks=size/BSIZE;
  printf(" no. of blocks = %u \n", nblocks);
	//store how many blocks are used per thread
	char* hash=malloc(sizeof(uint64_t));//will hold was is being returned
	
	//store how many blocks each thread will read per recursive call
	blocksper=nblocks/threads;
	printf("%" PRIu64 "\n",bytesLeft);
	pthread_t p;
	//create map and store into stack
	arr = mmap(NULL,size,PROT_READ,MAP_PRIVATE,fd,0);
	   if(arr== MAP_FAILED){
	     perror("map failed");
	       exit(EXIT_FAILURE);
	   }
	   //used to create first thread
	int x =0;
  double start = GetTime();

  
 	
  // calculate hash value of the input file
	Pthread_create(&p,NULL,tree,&x);	
	Pthread_join(p,(void**)&hash);

	 double end = GetTime();
	//print values, close file and return
  printf("hash value = %s \n",hash);
  printf("time taken = %f \n", (end - start));
  close(fd);
  return EXIT_SUCCESS;
}
void* tree(void* arg1)
{
	int *i = (int *) arg1;
	//current thread
	int currT = *i;
	char* tReturn= NULL;
	int next =(currT*2)+1;
	//Node has no children get hash of block and return
	if(next>(threads-1)&&(next+1)>(threads-1))
		{
		//check and make sure the read bytes by the map doesnt go past the end of the file
		
		//only necessary if it is a root
	uint64_t toRead;
		uint64_t checker = (BSIZE*currT*blocksper)+(blocksper*BSIZE);
		//set toRead to be how many bytes need to be read so reading doesnt go past end of file
		if(checker>bytesLeft)
		{
		toRead=bytesLeft-(uint64_t)(BSIZE*currT*blocksper);
		}
		//this is just to get tc4 1 thread to work
		else if(threads==1)
		toRead=bytesLeft;
		else 
		 toRead=(blocksper*BSIZE);
		 //compute hash
		uint32_t x=jenkins_one_at_a_time_hash(&arr[BSIZE*currT*blocksper],toRead);
		tReturn=malloc(sizeof(uint32_t));
		//convert to string and return
		sprintf(tReturn,"%u",x);
		pthread_exit(tReturn);
	
		}
		//node has one child which is a left child
	else if((next+1)>(threads-1))
	{
//		printf("One Child to create\n");
		char* lHash=malloc(sizeof(uint64_t));
		tReturn=malloc(2*(sizeof(uint32_t)));
		pthread_t leftChild;
		Pthread_create(&leftChild,NULL,tree,&next);
		 //compute hash for this tnum while waiting on child
		 uint32_t x=jenkins_one_at_a_time_hash(&arr[BSIZE*currT*blocksper],(blocksper*BSIZE));
	 //        bytesLeft-=(blocksper*BSIZE);
		 sprintf(tReturn,"%u",x);
		 //wait for child
		Pthread_join(leftChild,(void**)&lHash);
	//	printf("back to parent\n");
		//cat computed hash with the childs computed hash
		strcat(tReturn,lHash);
		//take the hash of the catted string
		x=jenkins_one_at_a_time_hash((uint8_t*)tReturn,strlen(tReturn));
		sprintf(tReturn,"%u",x);
		//return to parent
		pthread_exit(tReturn);
	}
	//two children
	else
	{
		//create memory and left and right child threads
		tReturn=malloc(3*(sizeof(uint32_t)));
		char * lHash=malloc(sizeof(uint32_t));
		char* rHash=malloc(sizeof(uint32_t));
		pthread_t lC,rC;
	
//		//create child processes
		Pthread_create(&lC,NULL,tree,&next);
		int tempHold=	next+1;
		Pthread_create(&rC,NULL,tree,&tempHold);
		//calc current threads hash while waiting
		 uint32_t x=jenkins_one_at_a_time_hash(&arr[BSIZE*currT*blocksper],(blocksper*BSIZE));
	//	bytesLeft-=(blocksper*BSIZE);
		sprintf(tReturn,"%u",x);
		 //wait for child threads
		Pthread_join(rC,(void**)&rHash);
		Pthread_join(lC,(void**)&lHash);
		//cat hash values
		strcat(tReturn,lHash);
		strcat(tReturn,rHash);
		//hash catted hashvalues
		x=jenkins_one_at_a_time_hash((uint8_t*)tReturn,strlen(tReturn));
		sprintf(tReturn,"%u",x);
		//return to parent
		pthread_exit(tReturn);
		}


pthread_exit(tReturn);
}
uint32_t 
jenkins_one_at_a_time_hash(const uint8_t* key, uint64_t length) 
{
  uint64_t i = 0;
  uint32_t hash = 0;

  while (i != length) {
    hash += key[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}


void 
Usage(char* s) 
{
  fprintf(stderr, "Usage: %s filename num_threads \n", s);
  exit(EXIT_FAILURE);
}
