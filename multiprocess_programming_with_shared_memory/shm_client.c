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
    return 0;
}
