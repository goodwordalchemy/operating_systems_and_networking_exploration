all: client server

client: client.c gwa_sockets.c 
	gcc -o client.out -Wall -g client.c gwa_sockets.c

server: server.c gwa_sockets.c gwa_threads.c
	gcc -o server.out -Wall -g server.c gwa_sockets.c gwa_threads.c

clean:
	rm -r *.out *.dSYM
