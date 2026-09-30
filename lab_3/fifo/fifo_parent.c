#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h> 
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#define FIFO_PATH "/tmp/my_fifo"

int main(void)
{
    if (mkfifo(FIFO_PATH, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
            return EXIT_FAILURE;
        }
    }

    time_t t = time(NULL);
    char *ts = ctime(&t);
    if (ts) ts[strcspn(ts, "\n")] = '\0';

    char msg[PIPE_BUF];
    int len = snprintf(msg, sizeof(msg), "pid=%d time=%s", (int)getpid(), ts ? ts : "?");
    if (len < 0 || (size_t)len >= sizeof(msg)) {
        fprintf(stderr, "message too long\n");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "[parent] pid=%d, waiting for reader...\n", (int)getpid());

    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        perror("open fifo");
        return EXIT_FAILURE;
    }

    ssize_t w = write(fd, msg, (size_t)len + 1);
    if (w < 0) {
        perror("write");
        close(fd);
        return EXIT_FAILURE;
    }
    close(fd);

    printf("[parent] sent: %s\n", msg);
    return EXIT_SUCCESS;
}