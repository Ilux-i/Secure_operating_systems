#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h>

#define IPC_CHANNEL_PATH "/tmp/my_fifo"

int main(void) {
    char rx_buffer[PIPE_BUF];
    
    fprintf(stdout, "[CHILD] Ожидание поступления данных через именованный канал...\n");

    int file_desc = open(IPC_CHANNEL_PATH, O_RDONLY);
    if (file_desc < 0) {
        perror("Ошибка открытия FIFO для чтения");
        return EXIT_FAILURE;
    }

    ssize_t received_bytes = read(file_desc, rx_buffer, sizeof(rx_buffer) - 1);
    if (received_bytes < 0) {
        perror("Сбой при чтении из FIFO");
        close(file_desc);
        return EXIT_FAILURE;
    }
    
    rx_buffer[received_bytes] = '\0';
    close(file_desc);

    time_t current_time = time(NULL);
    printf("[CHILD] Текущее системное время: %s", ctime(&current_time));
    printf("[CHILD] Получено от родителя: %s", rx_buffer);

    unlink(IPC_CHANNEL_PATH);

    return EXIT_SUCCESS;
}