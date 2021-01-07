#include <arpa/inet.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void logexit(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

void addrtostr(const struct sockaddr_in *addr, char *str, size_t strsize) {
  char addrstr[INET6_ADDRSTRLEN + 1] = "";
  uint16_t port = ntohs(addr->sin_port);
  inet_ntop(AF_INET, &(addr->sin_addr), addrstr, INET6_ADDRSTRLEN + 1);
  snprintf(str, strsize, "%s %hu", addrstr, port);
}

int subscribe(int client_socket, char *tag_name, char ***tags, int *tags_count, int ***subs, int **subs_count) {
  int i;
  int tag_index = -1;
  for (i = 0; i < *tags_count; i++) {
    if (strcmp(tag_name, (*tags)[i]) == 0) {
      tag_index = i;
    }
  }

  if (tag_index == -1) {
    tag_index = *tags_count;
    *tags_count = (*tags_count) + 1;

    *tags = realloc(*tags, *tags_count * sizeof(int *));
    *subs = realloc(*subs, *tags_count * sizeof(int *));
    *subs_count = realloc(*subs_count, *tags_count * sizeof(int));

    (*tags)[tag_index] = tag_name;
    (*subs_count)[tag_index] = 1;
    (*subs)[tag_index] = malloc(sizeof(int));
    (*subs)[tag_index][0] = client_socket;
    return 1;
  } else {
    int found_client = 0;
    for (i = 0; i < (*subs_count)[tag_index]; i++) {
      if ((*subs)[tag_index][i] == client_socket) {
        found_client = 1;
      }
    }
    if (found_client) {
      return 0;
    } else {
      (*subs_count)[tag_index] = ((*subs_count)[tag_index]) + 1;
      (*subs)[tag_index] = realloc((*subs)[tag_index], (*subs_count)[tag_index] * sizeof(int));
      ((*subs)[tag_index])[((*subs_count)[tag_index]) - 1] = client_socket;
      return 1;
    }
  }
}

int unsubscribe(int client_socket, char *tag_name, char ***tags, int *tags_count, int ***subs, int **subs_count) {
  int i;
  int tag_index = -1;
  for (i = 0; i < *tags_count; i++) {
    if (strcmp(tag_name, (*tags)[i]) == 0) {
      tag_index = i;
    }
  }

  if (tag_index == -1) {
    return 0;
  } else {
    int client_index = -1;
    for (i = 0; i < (*subs_count)[tag_index]; i++) {
      if ((*subs)[tag_index][i] == client_socket) {
        client_index = i;
      }
    }
    if (client_index != -1) {
      int *aux = malloc((*subs_count)[tag_index] * sizeof(int));
      int j = 0;
      for (i = 0; i < (*subs_count)[tag_index]; i++) {
        if ((*subs)[tag_index][i] != client_socket) {
          aux[j] = (*subs)[tag_index][i];
          j++;
        }
      }
      memcpy(((*subs)[tag_index]), aux, sizeof(*aux));
      (*subs_count)[tag_index] = (*subs_count)[tag_index] - 1;
      free(aux);
      return 1;
    } else {
      return 0;
    }
  }
}
