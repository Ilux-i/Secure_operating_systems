all: sender receiver

sender: sender.c shmem.h
	gcc -o sender sender.c

receiver: receiver.c shmem.h
	gcc -o receiver receiver.c

clean:
	rm -f sender receiver