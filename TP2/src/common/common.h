#pragma once

#include <arpa/inet.h>
#include <stdlib.h>

void logexit(const char *msg);

void addrtostr(const struct sockaddr_in *addr, char *str, size_t strsize);

int subscribe(int client_socket, char *tag_name, char ***tags, int *tags_count, int ***subs, int **subs_count);

int unsubscribe(int client_socket, char *tag_name, char ***tags, int *tags_count, int ***subs, int **subs_count);