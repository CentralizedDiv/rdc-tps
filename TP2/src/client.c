#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/common.h"

void usage(int argc, char **argv) {
  printf("usage: %s <server IP> <server port>\n", argv[0]);
  printf("example: %s 127.0.0.1 51511\n", argv[0]);
  exit(EXIT_FAILURE);
}

#define BUFSZ 1024

struct sock {
  int csock;
};

void *listenkb(void *data) {
  while (1) {
    struct sock *sdata = (struct sock *)data;
    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);

    printf("> ");
    fgets(buf, BUFSZ - 1, stdin);
    size_t count = send(sdata->csock, buf, strlen(buf) + 1, 0);
    if (count != strlen(buf) + 1) {
      logexit("send");
    }
  }

  pthread_exit(EXIT_SUCCESS);
}

void *listennw(void *data) {
  struct sock *sdata = (struct sock *)data;
  while (1) {
    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);

    size_t count = recv(sdata->csock, buf, BUFSZ, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("\r< %s> ", buf);
    if (count == 0) {
      break;
    }
  }

  close(sdata->csock);
  sdata->csock = -1;
  pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    usage(argc, argv);
  }

  // Connecting to the network
  struct sockaddr_in addr;
  uint16_t port = (uint16_t)atoi(argv[2]);
  port = htons(port);

  struct in_addr inaddr;
  if (inet_pton(AF_INET, argv[1], &inaddr)) {
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr = inaddr;
  }

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    logexit("socket");
  }

  if (connect(fd, (struct sockaddr *)(&addr), sizeof(addr)) == -1) {
    logexit("connect");
  }

  char addrstr[BUFSZ];
  addrtostr(&addr, addrstr, BUFSZ);
  printf("\n\t [Client Init] Connected to %s\n\n", addrstr);

  // END - Connecting to the network

  struct sock *sdata = malloc(sizeof(*sdata));
  sdata->csock = fd;

  // Listen to keyboard
  pthread_t kbtid;
  pthread_create(&kbtid, NULL, listenkb, sdata);

  // Listen to network
  pthread_t nwtid;
  pthread_create(&nwtid, NULL, listennw, sdata);

  while (1) {
    if (sdata->csock == -1) {
      printf("\r[Connection] Disconnected by %s\n", addrstr);
      break;
    }
  }

  exit(EXIT_SUCCESS);
}