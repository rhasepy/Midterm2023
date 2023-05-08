all: server.o client.o utils.o
	gcc -Wall server.o utils.o -o server -pthread -lrt
	gcc -Wall client.o utils.o -o client -pthread -lrt

server.o: server.c
	gcc -c -Wall -pedantic-errors server.c -std=gnu99 -pthread -lrt

client.o: client.c
	gcc -c -Wall -pedantic-errors client.c -std=gnu99

utils.o: utils.c
	gcc -c -Wall -pedantic-errors utils.c -std=gnu99

clean:
	rm -f *o && rm -f server && rm -f client