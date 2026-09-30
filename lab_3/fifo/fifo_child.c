#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#define FIFO_PATH "/tmp/my_fifo"

int main(void)
{
    char buf[PIPE_BUF];

    fprintf(stderr, "[child] waiting for fifo...\n");

    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) {
        perror("open fifo");
        return EXIT_FAILURE;
    }

    size_t total = 0;
    ssize_t n;
    while ((n = read(fd, buf + total, sizeof(buf) - 1 - total)) > 0) {
        total += (size_t)n;
        if (total >= sizeof(buf) - 1)
            break;
    }
    if (n < 0) {
        perror("read");
        close(fd);
        unlink(FIFO_PATH);
        return EXIT_FAILURE;
    }
    buf[total] = '\0';
    close(fd);
    unlink(FIFO_PATH);

    time_t t = time(NULL);
    char *ts = ctime(&t);
    if (ts) ts[strcspn(ts, "\n")] = '\0';

    printf("[child] local time : %s\n", ts ? ts : "?");
    printf("[child] got       : %s\n", buf);
    return EXIT_SUCCESS;
}