
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/common.h"

#define BUF_SIZE 500
#define LINE_END 10
#define SPACE 32
#define ADD_SUB 43
#define REMOVE_SUB 45
#define HASHTAG 35

#define UPPER_A 65
#define UPPER_Z 90
#define LOWER_A 97
#define LOWER_Z 122
#define MAX_ASCII 127

struct thread_data {
  int sock;
  struct sockaddr_in addr;
  int *tags_count;
  char **tags;
  int **subs;
  int *subs_count;
};

/**
 * Add value to arr and increase size of value is not already in arr
 * */
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

/**
 * Remove element arr[idx] and decrease size
 * */
void remove_element(int **arr, int idx, int *size) {
  int new_size = *size - 1;
  int *temp = malloc(new_size * sizeof(int));

  if (idx != 0) {
    memcpy(temp, *arr, idx * sizeof(int));
  }

  if (idx != new_size) {
    memcpy(temp + idx, *arr + idx + 1, (new_size - idx) * sizeof(int));  // copy everything AFTER the index
  }

  *arr = realloc(*arr, new_size * sizeof(int));
  memcpy(*arr, temp, new_size * sizeof(int));
  *size = (*size) - 1;
  free(temp);
}

void usage(int argc, char **argv) {
  printf("usage: %s <server port>\n", argv[0]);
  printf("example: %s 51511\n", argv[0]);
  exit(EXIT_FAILURE);
}

/**
 * Validate string searching for a valid tag, if it is valid, save it to buf and its size to size
 * 
 * Return 1 for valid and 0 if some invalid char was found in this tag 
 * */
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

/**
 * Search for subscription tags (+/-string-with-just-letters), subscribing or unsubscribing them 
 * to the client present in cdata->sock
 * 
 * Returns a feedback message
 * */
char *check_subscription_tags(char *message, struct thread_data *cdata) {
  // Check for +/-
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

/**
 * Search for tags (#string-with-just-letters) inside messages, saving them to
 * tags and how many to tags_count
 * */
void check_tags(char *message, char ***tags, int *tags_count) {
  int message_size = strlen(message);
  int i;
  for (i = 0; i < message_size; i++) {
    // If char is #
    if (message[i] == HASHTAG) {
      // Ignore everything before the #
      int sliced_message_size = message_size - i - 1;
      char *sliced_message = malloc(sliced_message_size * sizeof(char));
      memcpy(sliced_message, message + i + 1, sliced_message_size);

      // Validate if the string after # is a valid tag
      char *tag;
      if (validate_tag(&sliced_message, &tag, sliced_message_size)) {
        *tags_count = *tags_count + 1;
        *tags = realloc(*tags, *tags_count * sizeof(char *));
        (*tags)[*tags_count - 1] = tag;
        // If it is valid, keep looping the message after this tag
        i += strlen(tag);
      } else {
        char *next_space = strchr(sliced_message, SPACE);
        // If the invalid tag is the last thing on this message, just ignore
        if (next_space == NULL) {
          break;
        } else {
          // otherwise keep looping after the next space
          i += (int)(next_space - sliced_message);
        }
      }
    }
  }
}

/**
 * Exit the program if ##kill is found inside message
 * */
void check_kill(char *message) {
  // Remove \n
  message[strcspn(message, "\n")] = 0;
  if (strcmp(message, "##kill") == 0) {
    logexit("kill");
  }
}

/**
 * Return the size of the last message found on buffer, saving all messages found to
 * message and how many to messages_count
 * 
 * This return should be used as the offset for the next recv
 * */
int read_buffer(char *buffer, char ***messages, int *messages_count) {
  int i;
  int current_message_start = 0;
  int found_line_end = 0;
  for (i = 0; i < strlen(buffer); i++) {
    if (buffer[i] > MAX_ASCII) {
      // Something wrong, invalid character found
      return -1;
    }
    if (buffer[i] == LINE_END) {
      // End of message found
      *messages_count = (*messages_count) + 1;
      *messages = realloc(*messages, (*messages_count) * sizeof(char *));
      (*messages)[(*messages_count) - 1] = malloc((i - current_message_start) * sizeof(char));
      memcpy((*messages)[(*messages_count) - 1], buffer + current_message_start, i - current_message_start);
      current_message_start = i + 1;

      found_line_end = 1;
    }
  }

  if (found_line_end) {
    // If there is a unfinished message after the last message of this buffer
    if (buffer[strlen(buffer) - 1] != LINE_END) {
      // Copy it to the start of the buffer
      memmove(buffer, &buffer[current_message_start], strlen(buffer) - current_message_start);
      // Return where it ends
      return strlen(buffer) - current_message_start;
    } else {  // There is no splitted message
      return 0;
    }
  } else {
    if (i == BUF_SIZE - 1) {
      // Something wrong, \n was not found in this current message
      return -1;
    } else {
      // Splitted message
      return strlen(buffer);
    }
  }

  if (!found_line_end && i < BUF_SIZE - 1) {
    // Splitted message
    return 1;
  }
}

/**
 * Handles clients connections in parallel
 *  
 * @arg thread_data *data - pointer to thread_data cointaining tags and socket information
 * */
void *client_thread(void *data) {
  // Parse data
  struct thread_data *cdata = (struct thread_data *)data;
  struct sockaddr_in caddr = cdata->addr;

  char caddrstr[256];
  addrtostr(&caddr, caddrstr, 256);
  printf("[Client Connection] Client connected in %s\n", caddrstr);

  // Initialize message related variables
  int offset = 0;
  int messages_count = 0;
  char **messages = malloc(sizeof(char *));
  char buffer[BUF_SIZE];
  memset(buffer, 0, BUF_SIZE);

  while (1) {
    // Listen for messages from this client
    size_t count = recv(cdata->sock, buffer + offset, BUF_SIZE, 0);

    // If client disconnected
    if (count == 0) {
      break;
    }

    // When a message is received, handle it
    offset = read_buffer(buffer, &messages, &messages_count);

    // If something is wrong with the message, it could have invalid characters or be larger than the expected (500 bytes)
    if (offset == -1) {
      send(cdata->sock, "Invalid message!\n", 18, 0);
      messages_count = 0;
      offset = 0;
    } else {
      // Loop through all messages in this buffer
      int i;
      for (i = 0; i < messages_count; i++) {
        char *message = messages[i];
        printf("[Message] From: %s, %d bytes: %s\n", caddrstr, (int)count, message);

        // Check if it is a ##kill message
        check_kill(message);

        // Check if it is a subscription message
        char *feedback = check_subscription_tags(message, cdata);
        if (feedback != NULL) {
          send(cdata->sock, feedback, strlen(feedback), 0);
        }

        // Check if this message has tags
        int tags_count = 0;
        char **tags = malloc(sizeof(char *));
        check_tags(message, &tags, &tags_count);
        if (tags_count > 0) {
          // If it has, check which clients are subscribed to them
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
          strcat(message, "\n");

          // Loop through all clients subscribed to the tags found in this message sending it to them
          for (i = 0; i < clients_count; i++) {
            send(clients_to_notify[i], message, strlen(message), 0);
          }
        }

        // Clean the console
        fflush(stdout);
      }
    }

    // Reset state
    if (messages_count > 0) {
      messages_count = 0;
    }
    if (offset == 0) {
      memset(buffer, 0, BUF_SIZE);
    }
  }

  printf("[Client Connection] Client disconnected in %s\n", caddrstr);
  close(cdata->sock);

  // Remove client subscriptions
  int i, j;
  for (i = 0; i < *(cdata->tags_count); i++) {
    for (j = 0; j < cdata->subs_count[i]; j++) {
      if (cdata->subs[i][j] == cdata->sock) {
        remove_element(&(cdata->subs[i]), j, &(cdata->subs_count[i]));
      }
    }
  }
  pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage(argc, argv);
  }

  // Connecting to the network
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

  char addrstr[1024];
  addrtostr(&addr, addrstr, 1024);
  printf("\n\t [Server Init] Bound to %s, waiting connections...\n\n", addrstr);

  // END - Connecting to the network

  // Tags subscription variables
  int tags_count = 0;
  char **tags = malloc(tags_count * sizeof(char *));
  int **subs = malloc(tags_count * sizeof(int *));
  int *subs_count = malloc(tags_count * sizeof(int));

  // Listen for connections
  while (1) {
    struct sockaddr_in caddr;
    socklen_t caddrlen = sizeof(caddr);

    int sock = accept(fd, (struct sockaddr *)&caddr, &caddrlen);
    if (sock == -1) {
      logexit("accept");
    }

    // When a connection is requested create a thread passing the tags as arguments
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
