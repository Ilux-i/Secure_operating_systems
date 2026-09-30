#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>

#include "shmem.h"

static volatile sig_atomic_t stop_requested = 0;

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

    int shm_descriptor = shm_open(SHARED_MEM_PATH, O_RDWR, 0666);
    if (shm_descriptor < 0) {
        perror("Сначала запустите sender");
        return EXIT_FAILURE;
    }

    char *mapped_region = mmap(NULL, SHARED_MEM_CAPACITY, PROT_READ, MAP_SHARED, shm_descriptor, 0);
    close(shm_descriptor);

    if (mapped_region == MAP_FAILED) {
        perror("Ошибка отображения памяти (mmap)");
        shm_unlink(SHARED_MEM_PATH);
        return EXIT_FAILURE;
    }

    while (!stop_requested && memcmp(mapped_region, HALT_MESSAGE, SHARED_MEM_CAPACITY) != 0) {
        sleep(1);
        printf("[receiver] pid: %d, time: %ld\n", getpid(), time(NULL));
        printf("%s\n", mapped_region);
    }

    if (munmap(mapped_region, SHARED_MEM_CAPACITY) < 0) {
        perror("Ошибка снятия отображения (munmap)");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}