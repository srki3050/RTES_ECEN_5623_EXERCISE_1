#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#define COUNT  1000

typedef struct
{
    int threadIdx;
} threadParams_t;

pthread_t main_thread;
pthread_attr_t main_sched_attr;
int rt_max_prio, rt_min_prio, min;
struct sched_param main_param;

// POSIX thread declarations and scheduling attributes
//
pthread_t threads[2];
threadParams_t threadParams[2];


// Unsafe global
int gsum=0;

void *incThread(void *threadp)
{
    int i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

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

    for(i=0; i<COUNT; i++)
    {
        gsum=gsum-i;
        printf("Decrement thread idx=%d, gsum=%d\n", threadParams->threadIdx, gsum);
    }
}

void set_scheduler(void){
    int rc;
    
    pthread_attr_init(&main_sched_attr);
    pthread_attr_setinheritsched(&main_sched_attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&main_sched_attr, SCHED_FIFO);

    rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    main_param.sched_priority = rt_max_prio;
    rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);


    if (rc)
    {
       printf("ERROR; sched_setscheduler rc is %d\n", rc);
       perror("sched_setschduler"); exit(-1);
    }

    printf("After adjustments to scheduling policy:\n");
}

void print_scheduler(void)
{
   int schedType;
   /* Returns the current scheduling policy implemented for the current thread
    * If PID = 0, policy of the caling thread is retrieved.
    * if it returns -1 then the thread whose ID is pid could not be found
    */
   schedType = sched_getscheduler(getpid());
   /* getpid() - is a function that returns the process ID of the current process.
    */ 
   switch(schedType)
   {
     case SCHED_FIFO:
           printf("Pthread Policy is SCHED_FIFO\n");
           break;
     case SCHED_OTHER:
           printf("Pthread Policy is SCHED_OTHER\n");
       break;
     case SCHED_RR:
           printf("Pthread Policy is SCHED_RR\n");
           break;
     default:
       printf("Pthread Policy is UNKNOWN\n");
   }
}


int main (int argc, char *argv[])
{
   printf("Printing the current scheduling policy is\n");
   print_scheduler();
   set_scheduler();
   printf("Printing the current scheduling policy is\n");
   print_scheduler();
   //Declare an int variable rc 
   int rc;
   //Declare an int variable and initialize it to 0
   int i=0;
   /* typedef struct{
    *   int threadIdx;
    * }threadParams_t
    */
   /* pthread_t threads[2];
    * threadParams_t threadParams[2];
    */
    // store the current value of i in data structure
   threadParams[i].threadIdx=i;
   /*Creates a new thread of execution
   * Parameters - pthread_t threads (A thread object)
   *              attributes, (default here)
   *              A thread function that we have to run - incThread 
   *              Thread function takes one void pointer argument and returns a void pointer argument
   *              pass in the index value
   */
   pthread_create(&threads[i],   // pointer to thread descriptor
                  (void *)0,     // use default attributes
                  incThread, // thread function entry point
                  (void *)&(threadParams[i]) // parameters to pass in
                 );
   i++;
    //Perform the same activity for decrement thread so that they happen together
   threadParams[i].threadIdx=i;
   pthread_create(&threads[i], (void *)0, decThread, (void *)&(threadParams[i]));
    //Join both threads so that the data is printed according to both the schedules.
   for(i=0; i<2; i++)
     pthread_join(threads[i], NULL);

   printf("TEST COMPLETE\n");
}
