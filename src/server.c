
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/common.h"

#define BUFSZ 1024

void usage(int argc, char **argv) {
  printf("usage: %s <server port>\n", argv[0]);
  printf("example: %s 51511\n", argv[0]);
  exit(EXIT_FAILURE);
}

struct client_data {
  int sock;
  struct sockaddr_in addr;
};

void *client_thread(void *data) {
  struct client_data *cdata = (struct client_data *)data;
  struct sockaddr_in caddr = cdata->addr;

  char caddrstr[BUFSZ];
  addrtostr(&caddr, caddrstr, BUFSZ);
  printf("[Client Connection] Client connected in %s\n", caddrstr);

  while (1) {
    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    size_t count = recv(cdata->sock, buf, BUFSZ - 1, 0);
    if (count == 0) {
      break;
    }
    printf("[Message] From: %s, %d bytes: %s\n", caddrstr, (int)count, buf);

    sprintf(buf, "remote endpoint: %.1000s\n", caddrstr);
    count = send(cdata->sock, buf, strlen(buf) + 1, 0);
    if (count != strlen(buf) + 1) {
      logexit("send");
    }
  }

  printf("[Client Connection] Client disconnected in %s\n", caddrstr);
  close(cdata->sock);
  pthread_exit(EXIT_SUCCESS);
}

void subscribe() {
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage(argc, argv);
  }

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    logexit("socket");
  }

  int enable = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
    logexit("setsockopt");
  }

  struct sockaddr_in addr;
  uint16_t port = (uint16_t)atoi(argv[1]);
  port = htons(port);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = port;

  if (bind(fd, (struct sockaddr *)(&addr), sizeof(addr)) == -1) {
    logexit("bind");
  }

  if (listen(fd, 10) == -1) {
    logexit("listen");
  }

  char addrstr[BUFSZ];
  addrtostr(&addr, addrstr, BUFSZ);
  printf("\n\t [Server Init] Bound to %s, waiting connections...\n\n", addrstr);

  while (1) {
    struct sockaddr_in caddr;
    socklen_t caddrlen = sizeof(caddr);

    int sock = accept(fd, (struct sockaddr *)&caddr, &caddrlen);
    if (sock == -1) {
      logexit("accept");
    }

    struct client_data *cdata = malloc(sizeof(*cdata));
    cdata->sock = sock;
    memcpy(&(cdata->addr), &caddr, sizeof(caddr));

    pthread_t tid;
    pthread_create(&tid, NULL, client_thread, cdata);
  }

  exit(EXIT_SUCCESS);
}
