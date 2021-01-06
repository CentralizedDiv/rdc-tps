#include <arpa/inet.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void logexit(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_in *addr) {
  uint16_t port = (uint16_t)atoi(portstr);
  port = htons(port);

  struct in_addr inaddr;
  if (inet_pton(AF_INET, addrstr, &inaddr)) {
    addr->sin_family = AF_INET;
    addr->sin_port = port;
    addr->sin_addr = inaddr;
    return 0;
  }

  return -1;
}

void addrtostr(const struct sockaddr_in *addr, char *str, size_t strsize) {
  char addrstr[INET6_ADDRSTRLEN + 1] = "";
  uint16_t port = ntohs(addr->sin_port);
  inet_ntop(AF_INET, &(addr->sin_addr), addrstr, INET6_ADDRSTRLEN + 1);
  snprintf(str, strsize, "%s %hu", addrstr, port);
}
