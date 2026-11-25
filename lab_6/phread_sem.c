#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <semaphore.h>
#include <fcntl.h>

#define BUF_LEN 256

static char g_buf[BUF_LEN];
static sem_t *g_write_sem = NULL;
static sem_t *g_read_sem = NULL;
static char g_write_name[64];
static char g_read_name[64];
static volatile sig_atomic_t g_stop = 0;

static void on_signal(int signal_number)
{
    (void)signal_number;
    g_stop = 1;
    if (g_write_sem != NULL && g_write_sem != SEM_FAILED)
        sem_post(g_write_sem);
    if (g_read_sem != NULL && g_read_sem != SEM_FAILED)
        sem_post(g_read_sem);
}

static void *writer(void *unused)
{
    (void)unused;
    int number = 0;

    while (!g_stop) {
        if (sem_wait(g_write_sem) == -1 || g_stop)
            break;
        snprintf(g_buf, sizeof(g_buf), "%d", number++);
        printf("Записано: %s\n", g_buf);
        sem_post(g_read_sem);
        sleep(1);
    }
    return NULL;
}

static void *reader(void *unused)
{
    (void)unused;
    while (!g_stop) {
        if (sem_wait(g_read_sem) == -1 || g_stop)
            break;
        printf("Прочитано, TID: %lu, массив: %s\n",
               (unsigned long)pthread_self(), g_buf);
        sem_post(g_write_sem);
    }
    return NULL;
}

int main(void)
{
    struct sigaction action;
    pthread_t writer_id;
    pthread_t reader_id;

    memset(&action, 0, sizeof(action));
    action.sa_handler = on_signal;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    snprintf(g_write_name, sizeof(g_write_name), "/os_write_%d", getpid());
    snprintf(g_read_name, sizeof(g_read_name), "/os_read_%d", getpid());
    g_write_sem = sem_open(g_write_name, O_CREAT | O_EXCL, 0600, 1);
    if (g_write_sem == SEM_FAILED) {
        perror("sem_open writer");
        return EXIT_FAILURE;
    }
    g_read_sem = sem_open(g_read_name, O_CREAT | O_EXCL, 0600, 0);
    if (g_read_sem == SEM_FAILED) {
        perror("sem_open reader");
        sem_close(g_write_sem);
        sem_unlink(g_write_name);
        return EXIT_FAILURE;
    }

    g_buf[0] = '\0';
    puts("Запущен обмен между двумя потоками. Ctrl+C — остановка.");
    if (pthread_create(&writer_id, NULL, writer, NULL) != 0) {
        perror("pthread_create writer");
        return EXIT_FAILURE;
    }
    if (pthread_create(&reader_id, NULL, reader, NULL) != 0) {
        perror("pthread_create reader");
        g_stop = 1;
        sem_post(g_write_sem);
        pthread_join(writer_id, NULL);
        sem_close(g_write_sem);
        sem_close(g_read_sem);
        sem_unlink(g_write_name);
        sem_unlink(g_read_name);
        return EXIT_FAILURE;
    }

    pthread_join(writer_id, NULL);
    pthread_join(reader_id, NULL);
    sem_close(g_write_sem);
    sem_close(g_read_sem);
    sem_unlink(g_write_name);
    sem_unlink(g_read_name);
    return EXIT_SUCCESS;
}
