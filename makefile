all:	check

default: check
	
clean-worker:
	rm -rf worker.o
	
clean-master:
	rm -rf master.o

worker.o:	worker.c
	gcc -o worker.o worker.c -lm

master.o:	master.c
	gcc -o master.o master.c

sequential:	worker.o master.o
	./master.o --worker_path ./worker.o --wait_mechanism sequential -x 2 -n 10

select:	worker.o master.o
	./master.o --worker_path ./worker.o --wait_mechanism select -x 2 -n 10

poll:	worker.o master.o
	./master.o --worker_path ./worker.o --wait_mechanism poll -x 2 -n 10

epoll:	worker.o master.o
	./master.o --worker_path ./worker.o --wait_mechanism epoll -x 2 -n 10

worker:	worker.o
	./worker.o -x 2 -n 10

clean:	clean-worker clean-master

check:	clean-worker clean-master worker.o master.o sequential select poll epoll worker