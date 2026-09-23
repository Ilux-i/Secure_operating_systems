#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#define DURATION_SEC 10

static volatile sig_atomic_t is_termination_requested = 0;

static void cleanup_routine(void) {
    dprintf(STDOUT_FILENO, "[ATEXIT] Процесс с PID: %d (PPID: %d) завершает выполнение\n", getpid(), getppid());
}

static void handle_sigint(int signum) {
    dprintf(STDOUT_FILENO, "\n[SIGINT] Процесс %d перехватил сигнал SIGINT (код: %d)\n", getpid(), signum);
    is_termination_requested = 1;
}

static void handle_sigterm(int signum, siginfo_t *info, void *context) {
    (void)context;
    dprintf(STDOUT_FILENO, "\n[SIGTERM] Процесс %d перехватил сигнал SIGTERM (код: %d)\n", getpid(), signum);
    dprintf(STDOUT_FILENO, "[SIGTERM] Данные отправителя: PID=%d, UID=%d\n", info->si_pid, info->si_uid);
    is_termination_requested = 1;
}

int main(void) {
    atexit(cleanup_routine);

    signal(SIGINT, handle_sigint);

    struct sigaction term_action;
    memset(&term_action, 0, sizeof(term_action));
    term_action.sa_sigaction = handle_sigterm;
    term_action.sa_flags = SA_SIGINFO | SA_RESTART;
    
    if (sigaction(SIGTERM, &term_action, NULL) == -1) {
        perror("Ошибка при настройке sigaction для SIGTERM");
        return EXIT_FAILURE;
    }

    pid_t child_pid = fork();

    if (child_pid < 0) {
        perror("Не удалось создать процесс (fork)");
        return EXIT_FAILURE;
    } 
    
    if (child_pid == 0) {

        printf("[CHILD] Дочерний процесс запущен: PID=%d, PPID=%d\n", getpid(), getppid());
        
        for (int elapsed = 0; elapsed < DURATION_SEC; ++elapsed) {
            if (is_termination_requested) {
                printf("[CHILD] Получен сигнал завершения, аварийный выход\n");
                exit(EXIT_FAILURE);
            }
            
            sleep(1);
            printf("[CHILD] выполнение... %d/%d\n", elapsed + 1, DURATION_SEC);
        }
        
        printf("[CHILD] Штатное завершение работы\n");
        exit(EXIT_SUCCESS);
    } 
    
    printf("[PARENT] Родительский процесс: PID=%d\n", getpid());
    printf("[PARENT] Успешно создан дочерний процесс с PID=%d\n", child_pid);
    printf("[PARENT] Ожидание завершения дочернего процесса...\n");
    
    int child_status = 0;
    pid_t waited_pid = waitpid(child_pid, &child_status, 0);
    
    if (waited_pid == child_pid) {
        printf("[PARENT] Дочерний процесс (PID=%d) завершился\n", waited_pid);
        
        if (WIFEXITED(child_status)) {
            printf("[PARENT] Код возврата дочернего процесса: %d\n", WEXITSTATUS(child_status));
        } else if (WIFSIGNALED(child_status)) {
            printf("[PARENT] Дочерний процесс был прерван сигналом: %d\n", WTERMSIG(child_status));
        }
    } else {
        fprintf(stderr, "[PARENT] Ошибка при ожидании дочернего процесса\n");
    }
    
    printf("[PARENT] Завершение работы родительского процесса\n");
    return EXIT_SUCCESS;
}