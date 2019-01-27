all: client server

client: client.c common.c 
	gcc -o client.out -Wall -g client.c common.c

server: server.c common.c threads.c
	gcc -o server.out -Wall -g server.c common.c threads.c

clean:
	rm -r *.out *.dSYM
