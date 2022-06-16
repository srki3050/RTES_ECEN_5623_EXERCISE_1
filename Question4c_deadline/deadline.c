// https://www.i-programmer.info/programming/cc/13002-applying-c-deadline-scheduling.html?start=1
//

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <linux/sched.h>
#include <pthread.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/syscall.h>

struct sched_attr 
{
    uint32_t size;
    uint32_t sched_policy;
    uint64_t sched_flags;
    int32_t sched_nice;
    uint32_t sched_priority;
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};

static struct timespec current_time_capture = {0, 0};

int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags) 
{
    return syscall(__NR_sched_setattr, pid, attr, flags);
}

void * threadA(void *p) 
{
    struct sched_attr attr = 
    {
        .size = sizeof (attr),
        .sched_policy = SCHED_DEADLINE,
        .sched_runtime = 10 * 1000 * 1000,
        .sched_period = 2 * 1000 * 1000 * 1000,
        .sched_deadline = 11 * 1000 * 1000
    };

    sched_setattr(0, &attr, 0);

    for (;;) 
    {
        //addition of clock_gettime function
        clock_gettime(attr.sched_policy, &current_time_capture);
        printf("MY_CLOCK clock DT seconds = %ld, msec=%ld, usec=%ld, nsec=%ld, sec=%6.9lf\n", 
        current_time_capture.tv_sec, current_time_capture.tv_nsec/1000000, current_time_capture.tv_sec/1000, current_time_capture.tv_nsec);
        fflush(0);
        sched_yield();
    };
}


int main(int argc, char** argv) 
{
    pthread_t pthreadA;
    pthread_create(&pthreadA, NULL, threadA, NULL);
    pthread_exit(0);
    return (EXIT_SUCCESS);
}
