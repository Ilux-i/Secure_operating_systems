CC = gcc
CFLAGS = -Wall -Wextra -pthread

all: pthread_mutex

pthread_mutex: pthread_mutex.c
	$(CC) $(CFLAGS) -o pthread_mutex pthread_mutex.c

clean:
	rm -f pthread_mutex

.PHONY: all clean
