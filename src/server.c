
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/common.h"

#define BUFSZ 1024
#define LINE_END 10
#define SPACE 32
#define ADD_SUB 43
#define REMOVE_SUB 45
#define HASHTAG 35

#define UPPER_A 65
#define UPPER_Z 90
#define LOWER_A 97
#define LOWER_Z 122

struct thread_data {
  int sock;
  struct sockaddr_in addr;
  int *tags_count;
  char **tags;
  int **subs;
  int *subs_count;
};

void add_if_not_exists(int value, int **arr, int *size) {
  int i, exists = -1;
  for (i = 0; i < *size; i++) {
    if ((*arr)[i] == value) {
      exists = 1;
      break;
    }
  }
  if (exists == -1) {
    *arr = realloc(*arr, (*size + 1) * sizeof(int));
    (*arr)[*size] = value;
    *size = *size + 1;
  }
}

void usage(int argc, char **argv) {
  printf("usage: %s <server port>\n", argv[0]);
  printf("example: %s 51511\n", argv[0]);
  exit(EXIT_FAILURE);
}

int validate_tag(char **string, char **buf, size_t size) {
  int i;
  int tag_size;
  for (i = 0; i < size; i++) {
    if ((*string)[i] == SPACE || (*string)[i] == LINE_END) {
      tag_size = i;
      break;
    }
    if (  // If this char is not an ascii letter
        !(((*string)[i] >= UPPER_A && (*string)[i] <= UPPER_Z) ||
          ((*string)[i] >= LOWER_A && (*string)[i] <= LOWER_Z))) {
      return 0;
    }
  }
  *buf = malloc(tag_size * sizeof(char));
  memcpy(*buf, *string, tag_size);

  return 1;
}

char *check_subscription_tags(char *message, struct thread_data *cdata) {
  int prefix = -1;
  if (message[0] == ADD_SUB) {
    prefix = ADD_SUB;
  } else if (message[0] == REMOVE_SUB) {
    prefix = REMOVE_SUB;
  }
  if (prefix != -1) {
    char *space = strchr(message, SPACE);
    if (space == NULL) {
      int sliced_message_size = strlen(message) - 1;
      char *sliced_message = malloc(sliced_message_size * sizeof(char));
      memcpy(sliced_message, message + 1, sliced_message_size);

      char *tag;
      if (validate_tag(&sliced_message, &tag, sliced_message_size)) {
        if (prefix == ADD_SUB) {
          if (subscribe(cdata->sock, tag, &(cdata->tags), (cdata->tags_count), &(cdata->subs), &(cdata->subs_count))) {
            char *buf = malloc((13 + strlen(tag)) + 1 * sizeof(char));
            snprintf(buf, (13 + strlen(tag)) + 1 * sizeof(char), "subscribed +%s\n", tag);
            return buf;
          } else {
            char *buf = malloc((21 + strlen(tag)) + 1 * sizeof(char));
            snprintf(buf, (21 + strlen(tag)) + 1 * sizeof(char), "already subscribed +%s\n", tag);
            return buf;
          }
        } else if (prefix == REMOVE_SUB) {
          if (unsubscribe(cdata->sock, tag, &(cdata->tags), (cdata->tags_count), &(cdata->subs), &(cdata->subs_count))) {
            char *buf = malloc((15 + strlen(tag)) + 1 * sizeof(char));
            snprintf(buf, (15 + strlen(tag)) + 1 * sizeof(char), "unsubscribed -%s\n", tag);
            return buf;
          } else {
            char *buf = malloc((17 + strlen(tag)) + 1 * sizeof(char));
            snprintf(buf, (17 + strlen(tag)) + 1 * sizeof(char), "not subscribed -%s\n", tag);
            return buf;
          }
        }
      } else {
        return "Invalid message\n";
      }
    } else {
      return "Invalid message\n";
    }
  }
  return NULL;
}

void check_tags(char *message, char ***tags, int *tags_count) {
  int message_size = strlen(message);
  int i;
  for (i = 0; i < message_size; i++) {
    if (message[i] == HASHTAG) {
      int sliced_message_size = message_size - i - 1;
      char *sliced_message = malloc(sliced_message_size * sizeof(char));
      memcpy(sliced_message, message + i + 1, sliced_message_size);

      char *tag;
      if (validate_tag(&sliced_message, &tag, sliced_message_size)) {
        *tags_count = *tags_count + 1;
        *tags = realloc(*tags, *tags_count * sizeof(char *));
        (*tags)[*tags_count - 1] = tag;
        i += strlen(tag);
      } else {
        char *next_space = strchr(sliced_message, SPACE);
        if (next_space == NULL) {
          break;
        } else {
          i += (int)(next_space - sliced_message);
        }
      }
    }
  }
}

void check_kill(char *message) {
  message[strcspn(message, "\n")] = 0;
  if (strcmp(message, "##kill") == 0) {
    logexit("kill");
  }
}

void *client_thread(void *data) {
  struct thread_data *cdata = (struct thread_data *)data;
  struct sockaddr_in caddr = cdata->addr;

  char caddrstr[BUFSZ];
  addrtostr(&caddr, caddrstr, BUFSZ);
  printf("[Client Connection] Client connected in %s\n", caddrstr);

  while (1) {
    char message[BUFSZ];
    memset(message, 0, BUFSZ);
    size_t count = recv(cdata->sock, message, BUFSZ - 1, 0);
    if (count == 0) {
      break;
    }
    printf("[Message] From: %s, %d bytes: %s\n", caddrstr, (int)count, message);
    check_kill(message);

    char *feedback = check_subscription_tags(message, cdata);
    if (feedback != NULL) {
      send(cdata->sock, feedback, BUFSZ, 0);
    }

    int tags_count = 0;
    char **tags = malloc(sizeof(char *));
    check_tags(message, &tags, &tags_count);
    if (tags_count > 0) {
      int i, j, k;
      int clients_count = 0;
      int *clients_to_notify = malloc(sizeof(int));
      for (i = 0; i < tags_count; i++) {
        for (j = 0; j < *(cdata->tags_count); j++) {
          if (strcmp(cdata->tags[j], tags[i]) == 0) {
            for (k = 0; k < cdata->subs_count[j]; k++) {
              if (cdata->subs[j][k] != cdata->sock) {
                add_if_not_exists(cdata->subs[j][k], &clients_to_notify, &clients_count);
              }
            }
          }
        }
      }
      for (i = 0; i < clients_count; i++) {
        strcat(message, "\n");
        send(clients_to_notify[i], message, BUFSZ, 0);
      }
    }
    fflush(stdout);
  }

  printf("[Client Connection] Client disconnected in %s\n", caddrstr);
  close(cdata->sock);
  pthread_exit(EXIT_SUCCESS);
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

  int tags_count = 0;
  char **tags = malloc(tags_count * sizeof(char *));
  int **subs = malloc(tags_count * sizeof(int *));
  int *subs_count = malloc(tags_count * sizeof(int));

  while (1) {
    struct sockaddr_in caddr;
    socklen_t caddrlen = sizeof(caddr);

    int sock = accept(fd, (struct sockaddr *)&caddr, &caddrlen);
    if (sock == -1) {
      logexit("accept");
    }

    struct thread_data *cdata = malloc(sizeof(*cdata));
    cdata->sock = sock;
    cdata->tags_count = &tags_count;
    cdata->tags = tags;
    cdata->subs = subs;
    cdata->subs_count = subs_count;
    memcpy(&(cdata->addr), &caddr, sizeof(caddr));

    pthread_t tid;
    pthread_create(&tid, NULL, client_thread, cdata);
  }

  exit(EXIT_SUCCESS);
}
