/****************************************************************************/
/* Function: nanosleep and POSIX 1003.1b RT clock demonstration             */
/*                                                                          */
/* Sam Siewert - 02/05/2011                                                 */
/*                                                                          */
/****************************************************************************/

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#define NSEC_PER_SEC (1000000000)
#define NSEC_PER_MSEC (1000000)
#define NSEC_PER_USEC (1000)
#define ERROR (-1)
#define OK (0)
#define TEST_SECONDS (0)
#define TEST_NANOSECONDS (NSEC_PER_MSEC * 10)

void end_delay_test(void);
//Create data structures under type timespec to store sleep_time, sleep_requested, sleep_time

static struct timespec sleep_time = {0, 0};
static struct timespec sleep_requested = {0, 0};
static struct timespec remaining_time = {0, 0};

static unsigned int sleep_count = 0;

pthread_t main_thread;
pthread_attr_t main_sched_attr;
int rt_max_prio, rt_min_prio, min;
struct sched_param main_param;


void print_scheduler(void)
{
	/* Returns the current scheduling policy implemented for the current thread
	 * If PID = 0 (which can be obtained with getpid() function
	 * if PID = 0, policy of the calling thread is retrieved
	 * if it returns -1 then the thread whose ID is pid could not be found
	*/
   int schedType;

   schedType = sched_getscheduler(getpid());

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


double d_ftime(struct timespec *fstart, struct timespec *fstop)
{
  double dfstart = ((double)(fstart->tv_sec) + ((double)(fstart->tv_nsec) / 1000000000.0));
  double dfstop = ((double)(fstop->tv_sec) + ((double)(fstop->tv_nsec) / 1000000000.0));

  return(dfstop - dfstart); 
}


int delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t)
{
  int dt_sec=stop->tv_sec - start->tv_sec;
  int dt_nsec=stop->tv_nsec - start->tv_nsec;

  //printf("\ndt calcuation\n");

  // case 1 - less than a second of change
  if(dt_sec == 0)
  {
	  //printf("dt less than 1 second\n");

	  if(dt_nsec >= 0 && dt_nsec < NSEC_PER_SEC)
	  {
	          //printf("nanosec greater at stop than start\n");
		  delta_t->tv_sec = 0;
		  delta_t->tv_nsec = dt_nsec;
	  }

	  else if(dt_nsec > NSEC_PER_SEC)
	  {
	          //printf("nanosec overflow\n");
		  delta_t->tv_sec = 1;
		  delta_t->tv_nsec = dt_nsec-NSEC_PER_SEC;
	  }

	  else // dt_nsec < 0 means stop is earlier than start
	  {
	         printf("stop is earlier than start\n");
		 return(ERROR);  
	  }
  }

  // case 2 - more than a second of change, check for roll-over
  else if(dt_sec > 0)
  {
	  //printf("dt more than 1 second\n");

	  if(dt_nsec >= 0 && dt_nsec < NSEC_PER_SEC)
	  {
	          //printf("nanosec greater at stop than start\n");
		  delta_t->tv_sec = dt_sec;
		  delta_t->tv_nsec = dt_nsec;
	  }

	  else if(dt_nsec > NSEC_PER_SEC)
	  {
	          //printf("nanosec overflow\n");
		  delta_t->tv_sec = delta_t->tv_sec + 1;
		  delta_t->tv_nsec = dt_nsec-NSEC_PER_SEC;
	  }

	  else // dt_nsec < 0 means roll over
	  {
	          //printf("nanosec roll over\n");
		  delta_t->tv_sec = dt_sec-1;
		  delta_t->tv_nsec = NSEC_PER_SEC + dt_nsec;
	  }
  }

  return(OK);
}

static struct timespec rtclk_dt = {0, 0};
static struct timespec rtclk_start_time = {0, 0};
static struct timespec rtclk_stop_time = {0, 0};
static struct timespec delay_error = {0, 0};

#define MY_CLOCK CLOCK_REALTIME
//#define MY_CLOCK CLOCK_MONOTONIC
//#define MY_CLOCK CLOCK_MONOTONIC_RAW
//#define MY_CLOCK CLOCK_REALTIME_COARSE
//#define MY_CLOCK CLOCK_MONOTONIC_COARSE

#define TEST_ITERATIONS (100)

void *delay_test(void *threadID)
{
  int idx, rc;
  unsigned int max_sleep_calls=3;
  int flags = 0;
  struct timespec rtclk_resolution;

  sleep_count = 0;

  if(clock_getres(MY_CLOCK, &rtclk_resolution) == ERROR)
  {
      perror("clock_getres");
      exit(-1);
  }
  else
  {
      printf("\n\nPOSIX Clock demo using system RT clock with resolution:\n\t%ld secs, %ld microsecs, %ld nanosecs\n", rtclk_resolution.tv_sec, (rtclk_resolution.tv_nsec/1000), rtclk_resolution.tv_nsec);
  }

  for(idx=0; idx < TEST_ITERATIONS; idx++)
  {
      printf("test %d\n", idx);

      /* run test for defined seconds */
      sleep_time.tv_sec=TEST_SECONDS;
      sleep_time.tv_nsec=TEST_NANOSECONDS;
      sleep_requested.tv_sec=sleep_time.tv_sec;
      sleep_requested.tv_nsec=sleep_time.tv_nsec;

      /* start time stamp */ 
      clock_gettime(MY_CLOCK, &rtclk_start_time);

      /* request sleep time and repeat if time remains */
      do 
      {
          if(rc=nanosleep(&sleep_time, &remaining_time) == 0) break;
         
          sleep_time.tv_sec = remaining_time.tv_sec;
          sleep_time.tv_nsec = remaining_time.tv_nsec;
          sleep_count++;
      } 
      while (((remaining_time.tv_sec > 0) || (remaining_time.tv_nsec > 0))
		      && (sleep_count < max_sleep_calls));

      clock_gettime(MY_CLOCK, &rtclk_stop_time);

      delta_t(&rtclk_stop_time, &rtclk_start_time, &rtclk_dt);
      delta_t(&rtclk_dt, &sleep_requested, &delay_error);

      end_delay_test();
  }

}

void end_delay_test(void)
{
    double real_dt;
#if 0
  printf("MY_CLOCK start seconds = %ld, nanoseconds = %ld\n", 
         rtclk_start_time.tv_sec, rtclk_start_time.tv_nsec);
  
  printf("MY_CLOCK clock stop seconds = %ld, nanoseconds = %ld\n", 
         rtclk_stop_time.tv_sec, rtclk_stop_time.tv_nsec);
#endif

  real_dt=d_ftime(&rtclk_start_time, &rtclk_stop_time);
  printf("MY_CLOCK clock DT seconds = %ld, msec=%ld, usec=%ld, nsec=%ld, sec=%6.9lf\n", 
         rtclk_dt.tv_sec, rtclk_dt.tv_nsec/1000000, rtclk_dt.tv_nsec/1000, rtclk_dt.tv_nsec, real_dt);

#if 0
  printf("Requested sleep seconds = %ld, nanoseconds = %ld\n", 
         sleep_requested.tv_sec, sleep_requested.tv_nsec);

  printf("\n");
  printf("Sleep loop count = %ld\n", sleep_count);
#endif
  printf("MY_CLOCK delay error = %ld, nanoseconds = %ld\n", 
         delay_error.tv_sec, delay_error.tv_nsec);
}

#define RUN_RT_THREAD

void main(void)
{
   double real_dt;
   // Get the current time stamp at the start of execution
   clock_gettime(MY_CLOCK, &rtclk_start_time);
   
   int rc, scope;
   //before implementation of a scheduler, it is natural for a system for a SCHED_OTHER scheduling
   printf("Before adjustments to scheduling policy:\n");
   print_scheduler();
//If a process runs only when there is a command
#ifdef RUN_RT_THREAD
   /*	All threads are of equal priority
    * 	This command establishes a thread attribute object
    * 	Threads which are created with same set of characteristics without defining the characteristics in the thread.
   */
   pthread_attr_init(&main_sched_attr);
   /*	Sets the inherit scheduler attribute of the thread attribute.
    *	The inherit scheduler attribute determines whether a thread created using the thread attributes object attr will inherit
    * 	This thread is created using attr take their scheduling attributes from the values specified by the attributes project.
    */
   pthread_attr_setinheritsched(&main_sched_attr, PTHREAD_EXPLICIT_SCHED);
   /*	This function changes the scheduling to SCHED_FIFO  at this instant
    */
   pthread_attr_setschedpolicy(&main_sched_attr, SCHED_FIFO);
   // Set the maximum priority for a SCHED_FIFO
   rt_max_prio = sched_get_priority_max(SCHED_FIFO);
   // Set the minimum priority for a SCHED_FIFO
   rt_min_prio = sched_get_priority_min(SCHED_FIFO);
   
   main_param.sched_priority = rt_max_prio;
   rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
   /*	Obtain the status of the scheduler
    * 	if rc  = 0:	Scheduling policy updated
    * 	if rc != 0:	Error in updation of scheduling policy.
   */

   if (rc)
   {
       printf("ERROR; sched_setscheduler rc is %d\n", rc);
       perror("sched_setschduler"); exit(-1);
   }
   //	Now print the scheduler. The system should print SCHED_FIFO
   printf("After adjustments to scheduling policy:\n");
   print_scheduler();
   // Giving priority to the maximum event
   main_param.sched_priority = rt_max_prio;
   /* Function sets the scheduling parameter attributes of the thread attributes object referred to by attr to the values
    * The values specified in the buffer pointer to by param
    */
   pthread_attr_setschedparam(&main_sched_attr, &main_param);
   /* Now for new threads to be created for every process execution
    * We use the pthread_create function
    * if rc = 0.thread successfully created, else there is a failure in thread creation
    */ 
   rc = pthread_create(&main_thread, &main_sched_attr, delay_test, (void *)0);

   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror("pthread_create");
       exit(-1);
   }
   /* Now, the newly created threads should join the existing thread with the terminated thread
    * This function waits for the thread specified by thread to terminate
    * Returns immediately from pthread_join() if existing thread is completed
    */
   pthread_join(main_thread, NULL);
   /* When attributes are no longer required it should be destroyed using the destroy function.
    * Destroying thread attributes object has no effect on threads that were created using that object
    */
   if(pthread_attr_destroy(&main_sched_attr) != 0)
     perror("attr destroy");
#else
   delay_test((void *)0);
#endif

   printf("TEST COMPLETE\n");
   //End time stamp at the end of execution
   if(MY_CLOCK == CLOCK_MONOTONIC_RAW)
      printf("\n Total real time taken to execute in CLOCK_MONOTONIC_RAW clock is ");
   else if(MY_CLOCK == CLOCK_MONOTONIC)
      printf("\n Total real time taken to execute in CLOCK_MONOTONIC clock is ");
   else if(MY_CLOCK == CLOCK_REALTIME)
      printf("\n Total real time taken to execute in CLOCK_REALTIME clock is");
   clock_gettime(MY_CLOCK, &rtclk_stop_time);
   
   delta_t(&rtclk_stop_time, &rtclk_start_time, &rtclk_dt);
   
   real_dt=d_ftime(&rtclk_start_time, &rtclk_stop_time);
   printf("MY_CLOCK clock DT seconds = %ld, msec=%ld, usec=%ld, nsec=%ld, sec=%6.9lf\n", 
         rtclk_dt.tv_sec, rtclk_dt.tv_nsec/1000000, rtclk_dt.tv_nsec/1000, rtclk_dt.tv_nsec, real_dt);
}

