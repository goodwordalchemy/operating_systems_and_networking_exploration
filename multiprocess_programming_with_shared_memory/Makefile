all: shm_client.out shm_server.out

shm_server.out: shm_server.c common.c
	gcc -o shm_server.out -g -pthread shm_server.c common.c -lrt

shm_client.out: shm_client.c common.c
	gcc -o shm_client.out -g -pthread shm_client.c common.c -lrt

