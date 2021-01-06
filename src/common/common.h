#pragma once

#include <arpa/inet.h>
#include <stdlib.h>

void logexit(const char *msg);

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_in *addr);

void addrtostr(const struct sockaddr_in *addr, char *str, size_t strsize);