#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#define IPC_CHANNEL_PATH "/tmp/my_fifo"
#define DELAY_SEC 10

int main(void) {
    if (mkfifo(IPC_CHANNEL_PATH, 0666) == -1 && errno != EEXIST) {
        return EXIT_FAILURE;
    }

    pid_t current_pid = getpid();
    time_t current_time = time(NULL);
    char tx_buffer[PIPE_BUF];

    int formatted_len = snprintf(tx_buffer, sizeof(tx_buffer), "from pid: %d, time: %s", current_pid, ctime(&current_time));
    if (formatted_len < 0 || (size_t)formatted_len >= sizeof(tx_buffer)) {
        return EXIT_FAILURE;
    }

    printf("[PARENT] Идентификатор процесса: %d, время: %s", current_pid, ctime(&current_time));
    printf("[PARENT] Пауза %d секунд перед передачей данных...\n", DELAY_SEC);
    
    sleep(DELAY_SEC);

    int file_desc = open(IPC_CHANNEL_PATH, O_WRONLY);
    if (file_desc < 0) {
        perror("Ошибка открытия FIFO для записи");
        return EXIT_FAILURE;
    }

    if (write(file_desc, tx_buffer, strlen(tx_buffer) + 1) < 0) {
        perror("Сбой записи данных в FIFO");
        close(file_desc);
        return EXIT_FAILURE;
    }
    
    close(file_desc);
    printf("[PARENT] Данные успешно переданы в канал\n");

    return EXIT_SUCCESS;
}