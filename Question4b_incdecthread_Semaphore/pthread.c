#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <semaphore.h>
#include <unistd.h>

#define COUNT  1000
#define MAX_THREADS 2
typedef struct
{
    int threadIdx;
} threadParams_t;

// POSIX thread declarations and scheduling attributes
//
pthread_t threads[2];
threadParams_t threadParams[2];
sem_t startIOWorker[MAX_THREADS];

// Unsafe global
int gsum=0;

void *incThread(void *threadp)
{
    
    int i;
    threadParams_t *threadParams = (threadParams_t *)threadp;
    sem_wait (&(startIOWorker[threadParams->threadIdx]));
    for(i=0; i<COUNT; i++)
    {
        gsum=gsum+i;
        printf("Increment thread idx=%d, gsum=%d\n", threadParams->threadIdx, gsum);
    }
}


void *decThread(void *threadp)
{
    int i;
    threadParams_t *threadParams = (threadParams_t *)threadp;
    //sem_post (&(startIOWorker[i]));
    for(i=0; i<COUNT; i++)
    {
        gsum=gsum-i;
        printf("Decrement thread idx=%d, gsum=%d\n", threadParams->threadIdx, gsum);
    }
}




int main (int argc, char *argv[])
{
   
   int rc;
   int i=0;

   threadParams[i].threadIdx=i;
   if (sem_init (&(startIOWorker[i]), 0, 0)){
       printf ("Failed to initialize startIOWorker semaphore %d\n", i);
       exit (-1);
   }
   
   pthread_create(&threads[i],   // pointer to thread descriptor
                  (void *)0,     // use default attributes
                  incThread, // thread function entry point
                  (void *)&(threadParams[i]) // parameters to pass in
                 );
   i++;
   if (sem_init (&(startIOWorker[i]), 0, 0)){
       printf ("Failed to initialize startIOWorker semaphore %d\n", i);
       exit (-1);
   }
   threadParams[i].threadIdx=i;
   pthread_create(&threads[i], (void *)0, decThread, (void *)&(threadParams[i]));

   for(i=0; i<2; i++){
     pthread_join(threads[i], NULL);
     sem_destroy (&(startIOWorker[i]));
   } 

   printf("TEST COMPLETE\n");
}
