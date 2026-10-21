#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "shmem.h"

static volatile int stop_requested = 0;

static void catch_signal(int signum)
{
    (void)signum;
    stop_requested = 1;
}

static void install_signal_handlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = catch_signal;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}

int main(void)
{
    install_signal_handlers();

    key_t key = shmem_get_key();
    if (key == (key_t)-1) {
        perror("ftok");
        return EXIT_FAILURE;
    }

    int shm_id = shmget(key, SHARED_MEM_CAPACITY, 0);
    if (shm_id < 0) {
        perror("Сначала запустите sender (shmget)");
        return EXIT_FAILURE;
    }

    char *ptr = shmat(shm_id, NULL, SHM_RDONLY);
    if (ptr == (char *)-1) {
        perror("shmat");
        return EXIT_FAILURE;
    }

    while (!stop_requested &&
           strncmp(ptr, HALT_MESSAGE, strlen(HALT_MESSAGE)) != 0) {
        sleep(1);
        printf("[receiver] pid: %d, time: %ld\n", getpid(), time(NULL));
        printf("%s\n", ptr);
    }

    if (shmdt(ptr) < 0) {
        perror("shmdt");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}