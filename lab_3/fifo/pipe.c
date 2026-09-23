#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <string.h>

#define PARENT_DELAY_SEC 5

int main(void) {
    int ipc_descriptor[2];
    
    if (pipe(ipc_descriptor) != 0) {
        perror("Сбой при создании анонимного канала (pipe)");
        return EXIT_FAILURE;
    }

    pid_t child_process_id = fork();

    if (child_process_id < 0) {
        perror("Сбой клонирования процесса (fork)");
        return EXIT_FAILURE;
    }
    
    if (child_process_id == 0) {
        char rx_buffer[PIPE_BUF];
        
        close(ipc_descriptor[1]);
        
        ssize_t bytes_received = read(ipc_descriptor[0], rx_buffer, sizeof(rx_buffer) - 1);
        if (bytes_received < 0) {
            perror("Ошибка чтения данных из канала");
            close(ipc_descriptor[0]);
            return EXIT_FAILURE;
        }
        
        rx_buffer[bytes_received] = '\0';
        close(ipc_descriptor[0]);

        time_t now = time(NULL);
        printf("[CHILD]                                время: %s", ctime(&now));
        printf("[from parent in pipe] %s", rx_buffer);
        
        return EXIT_SUCCESS;
    } 
    else {
        pid_t current_pid = getpid();
        time_t now = time(NULL);
        char tx_buffer[PIPE_BUF];

        int msg_len = snprintf(tx_buffer, sizeof(tx_buffer), "from pid: %d, time: %s", current_pid, ctime(&now));
        if (msg_len < 0 || (size_t)msg_len >= sizeof(tx_buffer)) {
            perror("Ошибка подготовки буфера сообщения");
            return EXIT_FAILURE;
        }

        sleep(PARENT_DELAY_SEC);
        
        close(ipc_descriptor[0]);
        
        if (write(ipc_descriptor[1], tx_buffer, strlen(tx_buffer) + 1) < 0) {
            perror("Ошибка записи данных в канал");
            close(ipc_descriptor[1]);
            return EXIT_FAILURE;
        }
        close(ipc_descriptor[1]);

        int process_status;
        if (waitpid(child_process_id, &process_status, 0) == child_process_id) {
            if (WIFEXITED(process_status) && WEXITSTATUS(process_status) != 0) {
                fprintf(stderr, "Дочерний процесс завершился с кодом ошибки\n");
                return EXIT_FAILURE;
            }
        } else {
            perror("Ошибка ожидания завершения дочернего процесса");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}