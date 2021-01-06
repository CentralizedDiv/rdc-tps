#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Tag {
  char *name;
  int *subscriptions;
  int subscriptions_count;
} Tag;

int subscribe(int client_socket, char *tag_name, struct Tag **tags, int tags_count) {
  int i;
  int tag_index = -1;
  for (i = 0; i < tags_count; i++) {
    if (tags[i]->name != NULL) {
      if (strcmp(tag_name, tags[i]->name) == 0) {
        tag_index = i;
      }
    }
  }

  if (tag_index == -1) {
    tag_index = tags_count;
    tags_count = tags_count + 1;

    *tags = realloc(*tags, tags_count + 1 * sizeof(Tag));
    tags[tag_index] = malloc(sizeof(Tag));
    tags[tag_index]->name = tag_name;
    tags[tag_index]->subscriptions = malloc(sizeof(int));
    tags[tag_index]->subscriptions_count = 1;
    tags[tag_index]->subscriptions[0] = client_socket;

    return 1;
  } else {
    int found_client = 0;
    for (i = 0; i <= tags[tag_index]->subscriptions_count; i++) {
      if (tags[tag_index]->subscriptions[i] == client_socket) {
        found_client = 1;
      }
    }
    if (found_client) {
      return 0;
    } else {
      tags[tag_index]->subscriptions = realloc(tags[tag_index]->subscriptions, tags[tag_index]->subscriptions_count + 1 * sizeof(int));
      tags[tag_index]->subscriptions[tags[tag_index]->subscriptions_count] = client_socket;
      tags[tag_index]->subscriptions_count = tags[tag_index]->subscriptions_count + 1;
      return 1;
    }
  }
}

int unsubscribe(int client_socket, char *tag_name, struct Tag *tags, int *tags_count) {
  int i;
  int tag_index = -1;
  for (i = 0; i <= *tags_count; i++) {
    if (strcmp(tags[i].name, tag_name) == 0) {
      tag_index = i;
    }
  }

  if (tag_index == -1) {
    return 0;
  } else {
    int client_index = -1;
    for (i = 0; i <= tags[tag_index].subscriptions_count; i++) {
      if (tags[tag_index].subscriptions[i] == client_socket) {
        client_index = i;
      }
    }
    if (client_index != -1) {
      int *aux = malloc(tags[tag_index].subscriptions_count - 1 * sizeof(int));
      int j = 0;
      for (i = 0; i <= tags[tag_index].subscriptions_count; i++) {
        if (tags[tag_index].subscriptions[i] != client_socket) {
          aux[j] = tags[tag_index].subscriptions[i];
          j++;
        }
      }
      tags[tag_index].subscriptions = aux;
      tags[tag_index].subscriptions_count = tags[tag_index].subscriptions_count - 1;
      return 1;
    } else {
      return 0;
    }
  }
}

int main(int argc, char **argv) {
  int tags_count = 0;
  struct Tag *tags = malloc(tags_count * sizeof(Tag));
  subscribe(1, "dota", &tags, 0);
  subscribe(1, "overwatch", &tags, 1);
  subscribe(2, "dota", &tags, 2);
  subscribe(3, "dota", &tags, 2);
  struct Tag tag2 = tags[1];
  printf("\n%s, %ls\n", tag2.name, tag2.subscriptions);
  struct Tag tag1 = tags[0];
  printf("\n%s, %d\n", tag1.name, tag1.subscriptions_count);
}