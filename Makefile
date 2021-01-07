all:
	gcc -Wall -c ./src/common/common.c
	gcc -Wall ./src/client.c common.o -lpthread -o cliente
	gcc -Wall ./src/server.c common.o -lpthread -o servidor

clean:
	rm common.o cliente servidor
