#define SHM_NAME "faith_in_love"
#define PAGE_SIZE 4096
#define MAX_CLIENTS (PAGE_SIZE / 64 - 1)

pthread_mutex_t shmem_lock;

void *shmem;

typedef struct {
    int pid;
    char birth[25];
    char clientString[10];
    int elapsed_sec;
    double elapsed_msec;
} stats_t;

void print_client_str();

void read_shmem();
