all:
	gcc -Wall -c ./src/common/common.c
	gcc -Wall ./src/client.c common.o -lpthread -o client
	gcc -Wall ./src/server.c common.o -lpthread -o server

clean:
	rm common.o client server
