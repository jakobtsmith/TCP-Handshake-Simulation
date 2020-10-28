all: client.o server.o tcpseg.o
	gcc -o client client.o tcpseg.o
	gcc -o server server.o tcpseg.o

server.o: tcpseg.h server.c
	gcc -c server.c

client.o: tcpseg.h client.c
	gcc -c client.c

tcpseg.o: tcpseg.h tcpseg.c
	gcc -c tcpseg.c

clean:
	rm -f client
	rm -f server
	rm -f *.o
	rm -f *.out

