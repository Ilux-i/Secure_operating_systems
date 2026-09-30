#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

static void strip_nl(char *s)
{
    s[strcspn(s, "\n")] = '\0';
}

int main(void)
{
    int fd[2];

    if (pipe(fd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* child: reader */
        close(fd[1]);

        char buf[PIPE_BUF];
        size_t total = 0;
        ssize_t n;

        while ((n = read(fd[0], buf + total, sizeof(buf) - 1 - total)) > 0) {
            total += (size_t)n;
            if (total >= sizeof(buf) - 1)
                break;
        }
        if (n < 0) {
            perror("read");
            close(fd[0]);
            return EXIT_FAILURE;
        }
        buf[total] = '\0';
        close(fd[0]);

        time_t t = time(NULL);
        char *ts = ctime(&t);
        if (ts) strip_nl(ts);

        printf("[child] local time : %s\n", ts ? ts : "?");
        printf("[child] got       : %s\n", buf);
        return EXIT_SUCCESS;
    }

    close(fd[0]);

    time_t t = time(NULL);
    char *ts = ctime(&t);
    if (ts) strip_nl(ts);

    char msg[PIPE_BUF];
    int len = snprintf(msg, sizeof(msg), "pid=%d time=%s", (int)getpid(), ts ? ts : "?");
    if (len < 0 || (size_t)len >= sizeof(msg)) {
        fprintf(stderr, "message too long\n");
        close(fd[1]);
        return EXIT_FAILURE;
    }

    ssize_t w = write(fd[1], msg, (size_t)len + 1);
    if (w < 0) {
        perror("write");
        close(fd[1]);
        return EXIT_FAILURE;
    }
    close(fd[1]);

    int st = 0;
    if (waitpid(pid, &st, 0) == -1) {
        perror("waitpid");
        return EXIT_FAILURE;
    }
    if (WIFEXITED(st) && WEXITSTATUS(st) != 0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}