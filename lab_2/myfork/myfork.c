#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#define WORK_SECONDS 10

static volatile stop_flag = 0;

static void on_exit_print(void)
{
    printf("[exit] pid=%d ppid=%d\n", (int)getpid(), (int)getppid());
}

static void on_signal(int sig, siginfo_t *info, void *ctx)
{
    (void)ctx;
    stop_flag = 1;

    if (sig == SIGTERM && info != NULL) {
        printf("\n[signal] pid=%d sig=%d from pid=%d uid=%d\n",
               (int)getpid(), sig, (int)info->si_pid, (int)info->si_uid);
    } else {
        printf("\n[signal] pid=%d sig=%d\n", (int)getpid(), sig);
    }
}

static void install_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = on_signal;
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT,  &sa, NULL) == -1 ||
        sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    atexit(on_exit_print);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        install_handlers();

        printf("[child] start pid=%d ppid=%d\n", (int)getpid(), (int)getppid());

        for (int i = 0; i < WORK_SECONDS; i++) {
            if (stop_flag) {
                printf("[child] stopped by signal\n");
                return EXIT_FAILURE;
            }
            sleep(1);
            printf("[child] %d/%d\n", i + 1, WORK_SECONDS);
        }

        printf("[child] done\n");
        return EXIT_SUCCESS;
    }

    install_handlers();

    printf("[parent] pid=%d child=%d\n", (int)getpid(), (int)pid);

    int status = 0;
    pid_t r;
    do {
        r = waitpid(pid, &status, 0);
    } while (r == -1 && errno == EINTR);

    if (r == pid) {
        if (WIFEXITED(status))
            printf("[parent] child exit code=%d\n", WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("[parent] child killed by signal=%d\n", WTERMSIG(status));
    } else {
        perror("waitpid");
    }

    return EXIT_SUCCESS;
}