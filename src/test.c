#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Tag {
  char *name;
  int *subscriptions;
  int subscriptions_count;
} Tag;

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

    int a = (*subs_count)[0];
    int b = (*subs_count)[1];
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
      memcpy(((*subs)[tag_index]), aux, sizeof(aux));
      (*subs_count)[tag_index] = (*subs_count)[tag_index] - 1;
      free(aux);
      return 1;
    } else {
      return 0;
    }
  }
}

int main(int argc, char **argv) {
  int tags_count = 0;
  char **tags = malloc(tags_count * sizeof(char *));
  int **subs = malloc(tags_count * sizeof(int *));
  int *subs_count = malloc(tags_count * sizeof(int));

  subscribe(1, "dota", &tags, &tags_count, &subs, &subs_count);
  subscribe(5, "overwatch", &tags, &tags_count, &subs, &subs_count);
  subscribe(5, "overwatch", &tags, &tags_count, &subs, &subs_count);
  subscribe(50, "overwatch", &tags, &tags_count, &subs, &subs_count);
  subscribe(5, "dota", &tags, &tags_count, &subs, &subs_count);
  subscribe(3, "dota", &tags, &tags_count, &subs, &subs_count);
  unsubscribe(3, "dota", &tags, &tags_count, &subs, &subs_count);
  unsubscribe(3, "dota", &tags, &tags_count, &subs, &subs_count);
  subscribe(3, "dota", &tags, &tags_count, &subs, &subs_count);

  int j;
  for (j = 0; j < tags_count; j++) {
    printf("\n\tTag: %s\nClients Subscribed: ", tags[j]);
    int i;
    for (i = 0; i < subs_count[j]; i++) {
      printf("%d", subs[j][i]);
      if (i != subs_count[j] - 1) {
        printf(", ");
      }
    }
    printf("\n");
  }
}