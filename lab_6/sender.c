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
    int exit_code = EXIT_SUCCESS;
    key_t key;
    char *shared_data;
    struct sembuf access = {0, -1, SEM_UNDO};

    memset(&action, 0, sizeof(action));
    action.sa_handler = on_signal;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open: запустите только один sender");
        return EXIT_FAILURE;
    }
    if (ftruncate(fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        close(fd);
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }
    shared_data = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

    key = ftok(SEM_KEY_FILE, SEM_KEY_CODE);
    if (key == (key_t)-1) {
        perror("ftok");
        munmap(shared_data, SHM_SIZE);
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }
    sem_id = semget(key, 1, IPC_CREAT | 0666);
    if (sem_id == -1 || semctl(sem_id, 0, SETVAL, 1) == -1) {
        perror("semget/semctl");
        if (sem_id != -1)
            semctl(sem_id, 0, IPC_RMID);
        munmap(shared_data, SHM_SIZE);
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

    printf("Sender запущен, pid=%d. Ctrl+C — остановка.\n", getpid());
    while (!g_stop) {
        if (semop(sem_id, &access, 1) == -1) {
            if (!g_stop)
                perror("semop wait");
            break;
        }
        snprintf(shared_data, SHM_SIZE, "pid=%d; время=%ld",
                 getpid(), (long)time(NULL));
        access.sem_op = 1;
        if (semop(sem_id, &access, 1) == -1) {
            perror("semop post");
            exit_code = EXIT_FAILURE;
            break;
        }
        access.sem_op = -1;
        sleep(3);
    }

    semctl(sem_id, 0, IPC_RMID);
    munmap(shared_data, SHM_SIZE);
    shm_unlink(SHM_NAME);
    return exit_code;
}
