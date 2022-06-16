#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>

#define NUM_THREADS 12

typedef struct
{
    int threadIdx;
} threadParams_t;


// POSIX thread declarations and scheduling attributes
//
pthread_t threads[NUM_THREADS];
threadParams_t threadParams[NUM_THREADS];


void *counterThread(void *threadp)
{
    int sum=0, i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    for(i=1; i < (threadParams->threadIdx)+1; i++)
        sum=sum+i;
 
    printf("Thread idx=%d, sum[0...%d]=%d\n", 
           threadParams->threadIdx,
           threadParams->threadIdx, sum);
}


int main (int argc, char *argv[])
{
   //A function call to get the current scheduling mechanism
   int schedType = sched_getscheduler(getpid());
   if(schedType == SCHED_OTHER)
      printf("The current scheduling policy is SCHED_OTHER\n");
   else if(schedType == SCHED_FIFO)
      printf("The current scheduling policy is SCHED_FIFO\n");
   else if(schedType == SCHED_RR)
      printf("The current scheduling policy is SCHED_RR\n");
  
   int rc;
   int i;

   for(i=0; i < NUM_THREADS; i++)
   {
       threadParams[i].threadIdx=i;

       pthread_create(&threads[i],   // pointer to thread descriptor
                      (void *)0,     // use default attributes
                      counterThread, // thread function entry point
                      (void *)&(threadParams[i]) // parameters to pass in
                     );
   }

   for(i=0;i<NUM_THREADS;i++)
       pthread_join(threads[i], NULL);

   printf("TEST COMPLETE\n");
}
