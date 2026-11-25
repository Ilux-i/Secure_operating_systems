#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "shmem.h"

static volatile sig_atomic_t g_stop = 0;

static void on_signal(int signal_number)
{
    (void)signal_number;
    g_stop = 1;
}

int main(void)
{
    struct sigaction action;
    int fd;
    int sem_id;
    key_t key;
    char *shared_data;
    struct sembuf access = {0, -1, SEM_UNDO};

    memset(&action, 0, sizeof(action));
    action.sa_handler = on_signal;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (fd == -1) {
        perror("Сначала запустите sender");
        return EXIT_FAILURE;
    }
    shared_data = mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }

    key = ftok(SEM_KEY_FILE, SEM_KEY_CODE);
    if (key == (key_t)-1) {
        perror("ftok");
        munmap(shared_data, SHM_SIZE);
        return EXIT_FAILURE;
    }
    sem_id = semget(key, 1, 0666);
    if (sem_id == -1) {
        perror("semget");
        munmap(shared_data, SHM_SIZE);
        return EXIT_FAILURE;
    }

    puts("Receiver запущен. Ctrl+C — остановка.");
    while (!g_stop) {
        if (semop(sem_id, &access, 1) == -1) {
            if (!g_stop && errno != EIDRM)
                perror("semop wait");
            break;
        }
        printf("[receiver] pid=%d, время=%ld\nполучено: %s\n",
               getpid(), (long)time(NULL), shared_data);
        access.sem_op = 1;
        if (semop(sem_id, &access, 1) == -1)
            break;
        access.sem_op = -1;
        sleep(1);
    }

    munmap(shared_data, SHM_SIZE);
    return EXIT_SUCCESS;
}
