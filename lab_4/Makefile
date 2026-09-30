all: sender receiver

sender: sender.c shmem.h
	-o sender sender.c

receiver: receiver.c shmem.h
	-o receiver receiver.c

clean:
	rm -f sender receiver