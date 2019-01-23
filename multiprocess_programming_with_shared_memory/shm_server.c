#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

pthread_mutexattr_t shmem_lock_attr;

static void exit_handler(int sig){
    if (munmap(shmem, PAGE_SIZE) == -1){
        perror("munmap");
        exit(1);
    }

    if (shm_unlink(SHM_NAME) == -1){
       perror("shm_unlink");
       exit(1);
    }
    printf("successfully exiting\n");
    exit(0);
}

int main(int argc, char *argv[]) {
   int fd, iter;
   struct sigaction act;

   memset(&act, 0, sizeof(act));
   act.sa_handler = &exit_handler;

   if ((sigaction(SIGTERM, &act, NULL) < 0) || (sigaction(SIGINT, &act, NULL) < 0)) {
       perror("sigaction");
       return 1;
   }

   if (pthread_mutexattr_init(&shmem_lock_attr) != 0){
       perror("pthread_mutexattr_init");
       return 1;
   }

   if (pthread_mutexattr_setpshared(&shmem_lock_attr, PTHREAD_PROCESS_SHARED) != 0){
       perror("pthread_mutexattr_setpshared");
       return 1;
   }

   if (pthread_mutex_init(&shmem_lock, &shmem_lock_attr) != 0){
       perror("pthread_mutex_init");
       return 1;
   }


   if ((fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0660)) == -1){
       perror("shm_open");
       return 1;
   }

   if (ftruncate(fd, PAGE_SIZE) == -1){
       perror("ftruncate");
       return 1;
   }

   if ((shmem = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED){
       perror("mmap");
       return 1;
   }

   while(1){
       iter++;
       if (sleep(1) != 0)
           return 1;
       read_shmem(iter, shmem);
   }


   return 0;
}
