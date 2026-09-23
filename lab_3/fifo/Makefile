all: fifo_child fifo_parent pipe

fifo_child: fifo_child.c
	gcc -o fifo_child fifo_child.c

fifo_parent: fifo_parent.c
	gcc -o fifo_parent fifo_parent.c

pipe: pipe.c
	gcc -o pipe pipe.c

clean:
	rm -f fifo_child fifo_parent pipe