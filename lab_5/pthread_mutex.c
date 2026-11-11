#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>

#define BUF_LEN 256
#define READ_CNT 10
#define LAST_VALUE 10

static char g_buf[BUF_LEN];
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_stop;
static int g_writer_done;
static int g_finished_readers;

static void *producer(void *unused)
{
    (void)unused;

    for (int number = 1; number <= LAST_VALUE; ++number) {
        pthread_mutex_lock(&g_lock);
        if (g_stop) {
            pthread_mutex_unlock(&g_lock);
            return NULL;
        }

        snprintf(g_buf, sizeof(g_buf), "%d", number);
        g_writer_done = (number == LAST_VALUE);
        pthread_mutex_unlock(&g_lock);
        if (number < LAST_VALUE)
            sleep(1);
    }
    return NULL;
}

static void *consumer(void *arg)
{
    int reader = (int)(intptr_t)arg;
    sleep(1);

    for (;;) {
        pthread_mutex_lock(&g_lock);
        if (g_stop) {
            pthread_mutex_unlock(&g_lock);
            return NULL;
        }

        printf("[ЧИТАТЕЛЬ] TID: %d, массив: %s\n", reader, g_buf);
        if (g_writer_done) {
            int last_reader = (++g_finished_readers == READ_CNT);
            pthread_mutex_unlock(&g_lock);
            if (last_reader)
                kill(getpid(), SIGTERM);
            return NULL;
        }

        pthread_mutex_unlock(&g_lock);
        usleep(750000);
    }
}

int main(void)
{
    sigset_t signals;
    int signal_number;
    pthread_t writer;
    pthread_t readers[READ_CNT];

    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGTERM);
    int error = pthread_sigmask(SIG_BLOCK, &signals, NULL);
    if (error != 0) {
        fprintf(stderr, "pthread_sigmask: %s\n", strerror(error));
        return EXIT_FAILURE;
    }

    puts("Запущено 10 читателей и 1 писатель. Ctrl+C — остановка.");

    for (int i = 0; i < READ_CNT; ++i) {
        error = pthread_create(&readers[i], NULL, consumer,
                               (void *)(intptr_t)(i + 1));
        if (error != 0) {
            fprintf(stderr, "pthread_create reader: %s\n", strerror(error));
            return EXIT_FAILURE;
        }
    }
    error = pthread_create(&writer, NULL, producer, NULL);
    if (error != 0) {
        fprintf(stderr, "pthread_create writer: %s\n", strerror(error));
        return EXIT_FAILURE;
    }

    error = sigwait(&signals, &signal_number);
    if (error != 0)
        fprintf(stderr, "sigwait: %s\n", strerror(error));

    pthread_mutex_lock(&g_lock);
    g_stop = 1;
    pthread_mutex_unlock(&g_lock);

    pthread_join(writer, NULL);
    for (int i = 0; i < READ_CNT; ++i)
        pthread_join(readers[i], NULL);
    return error == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
