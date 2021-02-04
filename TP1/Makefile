all:
	mkdir -p ./build
	gcc -Wall -c ./src/common/common.c -o ./build/common.o
	gcc -Wall ./src/client.c ./build/common.o -lpthread -o ./build/client
	gcc -Wall ./src/server.c ./build/common.o -lpthread -o ./build/server

clean:
	rm -Rf ./build
