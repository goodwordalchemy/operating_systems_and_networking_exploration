#include <stdio.h>
#include <sys/types.h>

#include "common.h"

void print_client_str(int iter, stats_t *client){
    printf("iteration: %d, pid : %d, ", iter, client->pid);
    printf("birth : %s, ", client->birth);
    printf("elapsed : %d s %0.4f ms, ", client->elapsed_sec, client->elapsed_msec);
    printf("%s\n", client->clientString);
}

void read_shmem(int iter, void *shmem){
    int i, num_clients;
    stats_t *cur_client;
	size_t stats_t_size = sizeof(stats_t);
    for (i = 0; i < MAX_CLIENTS; i++){
        cur_client = ((stats_t*) (shmem + i * stats_t_size));
        if (cur_client->pid != 0){
            print_client_str(iter, cur_client);
            num_clients++;
        }
	}
    if (num_clients == 0)
        printf("iter: %d, There are no clients yet.\n", iter);
}
