#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
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

    int shm_descriptor = shm_open(SHARED_MEM_PATH, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_descriptor < 0) {
        if (errno == EEXIST) {
            fprintf(stderr, "Область памяти уже существует: /dev/shm%s\n", SHARED_MEM_PATH);
            return EXIT_FAILURE;
        }
        perror("Не удалось создать общую память (shm_open)");
        return EXIT_FAILURE;
    }

    if (ftruncate(shm_descriptor, SHARED_MEM_CAPACITY) < 0) {
        perror("Ошибка установки размера (ftruncate)");
        close(shm_descriptor);
        shm_unlink(SHARED_MEM_PATH);
        return EXIT_FAILURE;
    }

    char *mapped_ptr = mmap(NULL, SHARED_MEM_CAPACITY, PROT_WRITE, MAP_SHARED, shm_descriptor, 0);
    close(shm_descriptor);

    if (mapped_ptr == MAP_FAILED) {
        perror("Ошибка отображения памяти (mmap)");
        shm_unlink(SHARED_MEM_PATH);
        return EXIT_FAILURE;
    }

    printf("Нажмите Ctrl+C для остановки записи в /dev/shm%s\n", SHARED_MEM_PATH);

    while (!stop_requested) {
        snprintf(mapped_ptr, SHARED_MEM_CAPACITY, "[sender] pid: %d, time: %ld\n", getpid(), time(NULL));
        sleep(1);
    }

    snprintf(mapped_ptr, SHARED_MEM_CAPACITY, HALT_MESSAGE);

    int cleanup_error = 0;

    if (munmap(mapped_ptr, SHARED_MEM_CAPACITY) < 0) {
        perror("Ошибка снятия отображения (munmap)");
        cleanup_error = 1;
    }
    if (shm_unlink(SHARED_MEM_PATH) < 0) {
        perror("Ошибка удаления общей памяти (shm_unlink)");
        cleanup_error = 1;
    }

    if (cleanup_error) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}