#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
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

    int shm_id = shmget(key, SHARED_MEM_CAPACITY, IPC_CREAT | IPC_EXCL | 0666);
    if (shm_id < 0) {
        if (errno == EEXIST) {
            fprintf(stderr, "Область памяти уже существует (key=%d)\n", (int)key);
            return EXIT_FAILURE;
        }
        perror("shmget");
        return EXIT_FAILURE;
    }

    char *ptr = shmat(shm_id, NULL, 0);
    if (ptr == (char *)-1) {
        perror("shmat");
        shmctl(shm_id, IPC_RMID, NULL);
        return EXIT_FAILURE;
    }

    printf("Нажмите Ctrl+C для остановки записи (shmid=%d)\n", shm_id);

    while (!stop_requested) {
        snprintf(ptr, SHARED_MEM_CAPACITY,
                 "[sender] pid: %d, time: %ld\n", getpid(), time(NULL));
        sleep(1);
    }

    snprintf(ptr, SHARED_MEM_CAPACITY, HALT_MESSAGE);

    int rc = EXIT_SUCCESS;

    if (shmdt(ptr) < 0) {
        perror("shmdt");
        rc = EXIT_FAILURE;
    }
    if (shmctl(shm_id, IPC_RMID, NULL) < 0) {
        perror("shmctl(IPC_RMID)");
        rc = EXIT_FAILURE;
    }

    return rc;
}