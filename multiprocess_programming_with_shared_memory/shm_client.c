#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

stats_t *cur_client;

static void exit_handler(int sig){
    if (pthread_mutex_lock(&shmem_lock) != 0){
        perror("pthread_mutex_lock");
        exit(1);
    }

    memset(cur_client, 0, sizeof(stats_t));
    
    if (pthread_mutex_unlock(&shmem_lock) != 0){
        perror("pthread_mutex_lock");
        exit(1);
    }
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

int assign_client_address(void *shmem, stats_t **client_address){
    int i;
	size_t stats_t_size = sizeof(stats_t);
    stats_t *cur_client;
    for (i = 0; i < MAX_CLIENTS; i++){
        cur_client = ((stats_t*) (shmem + i * stats_t_size));
        if (cur_client->pid == 0){
            *client_address = cur_client;

            return 0;
        }
	}
    printf("There are no available spots.\n");

    return 1;
}

/* typedef struct { */
/*     int pid; */
/*     char birth[25]; */
/*     char clientString[10]; */
/*     int elapsed_sec; */
/*     double elapsed_msec; */
/* } stats_t; */
void birthday(char *buffer){
    time_t curtime;

    time(&curtime);

    snprintf(buffer, 20, "%s", ctime(&curtime));
}

void assign_client_stats(stats_t *cur_client, char *client_string){
    struct timeval tv;
    unsigned long usec;

    cur_client->pid = getpid();
    birthday(cur_client->birth);
    snprintf(cur_client->clientString, 10, "%s", client_string);

    gettimeofday(&tv, NULL);
    cur_client->elapsed_sec = (int) tv.tv_sec;
    usec = tv.tv_usec;
    cur_client->elapsed_msec = (double) (usec / 1000);
}

int main(int argc, char *argv[]) {
    int fd, iter;
    struct sigaction act;

    if (argc != 2){
        puts("usage: include client string argument");
        exit(1);
    }

    memset(&act, 0, sizeof(act));
    act.sa_handler = &exit_handler;

    if ((sigaction(SIGTERM, &act, NULL) < 0) || (sigaction(SIGINT, &act, NULL) < 0)) {
        perror("sigaction");
        return 1;
    }

    if ((fd = shm_open(SHM_NAME, O_RDWR, 0660)) == -1){
        perror("shm_open");
        return 1;
    }

    if ((shmem = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED){
        perror("mmap");
        return 1;
    }

    if (pthread_mutex_lock(&shmem_lock) != 0){
        perror("pthread_mutex_lock");
        return 1;
    }

    if (assign_client_address(shmem, &cur_client) == -1){
        perror("assign_client_address");
        return 1;
    }

    assign_client_stats(cur_client, argv[1]);
    
    if (pthread_mutex_unlock(&shmem_lock) != 0){
        perror("pthread_mutex_lock");
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
