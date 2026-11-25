CC = gcc
CFLAGS = -Wall -Wextra -pthread

all: phread_sem sender receiver

sender: sender.c shmem.h
	$(CC) $(CFLAGS) -o sender sender.c

receiver: receiver.c shmem.h
	$(CC) $(CFLAGS) -o receiver receiver.c

phread_sem: phread_sem.c
	$(CC) $(CFLAGS) -o phread_sem phread_sem.c

clean:
	rm -f phread_sem sender receiver

.PHONY: all clean
