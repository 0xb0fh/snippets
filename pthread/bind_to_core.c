/* gcc -Wall -o bind_to_core bind_to_core.c -lpthread */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sched.h>

#define NUM_THREADS     2
#define CPU_ID          0

#define fatal(msg, err)                         \
   do {                                         \
      errno = err;                              \
      perror(msg);                              \
      exit(EXIT_FAILURE);                       \
   } while (0)                 

int bind_to_core(int core_id) 
{
   cpu_set_t cpuset;

   CPU_ZERO(&cpuset);
   CPU_SET(core_id, &cpuset);
    
   return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

void *thread_func(void *parms)
{
   pid_t tid;
   int err;
   int core_num = *((int *)parms);
   
   tid = syscall(SYS_gettid); // gettid();
   printf("Setting TID: #%ld on CPU %d\n", (long)tid, core_num);
   
   if((err = bind_to_core(core_num)) != 0){
      if(err == EINVAL)
         fatal("Wrong CPUid", err);

      fatal("Unable to set affinity", err);
   }

   pthread_exit(NULL);
}

int main (int argc, char **argv)
{
   pthread_t threads[NUM_THREADS];
   int ret_val;
   int c_id = CPU_ID;
   int i;

   for(i = 0; i < NUM_THREADS; i++){
      printf("creating thread %d\n", i);
      ret_val = pthread_create(&threads[i], NULL, thread_func, (void *)&c_id);

      if (ret_val)
         fatal("ERROR; return value: %d\n", ret_val);
   }

   for(i = 0; i < NUM_THREADS; i++)
      pthread_join(threads[i], NULL);

   return 0;
}
