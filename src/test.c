#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UPPER_A 65
#define UPPER_Z 90
#define LOWER_A 97
#define LOWER_Z 122
#define LINE_END 10
#define SPACE 32
#define ADD_SUB 43
#define REMOVE_SUB 45
#define HASHTAG 35

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

// int validateTag(char **string, char **buf, size_t size) {
//   int i;
//   int delimiter = -1;
//   if ((*string)[size - 1] == SPACE) {
//     delimiter = SPACE;
//   } else if ((*string)[size - 1] == LINE_END) {
//     delimiter = LINE_END;
//   }

//   if (delimiter != -1) {
//     for (i = 0; i < size - 1; i++) {
//       if (  // If this char is not an ascii letter
//           !(((*string)[i] >= UPPER_A && (*string)[i] <= UPPER_Z) ||
//             ((*string)[i] >= LOWER_A && (*string)[i] <= LOWER_Z))) {
//         return 0;
//       }
//     }
//     memcpy(*buf, *string, size - 1);
//   } else {
//     return 0;
//   }
//   return 1;
// }

int validateTag(char **string, char **buf, size_t size) {
  int i;
  int tag_size;
  for (i = 0; i < size - 1; i++) {
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

int main(int argc, char **argv) {
  char *message = "cole braço! #dota #overwatch #asad";
  int message_size = strlen(message);
  int i;
  for (i = 0; i < message_size; i++) {
    if (message[i] == HASHTAG) {
      int sliced_message_size = message_size - i - 1;
      char *sliced_message = malloc(sliced_message_size * sizeof(char));
      memcpy(sliced_message, message + i + 1, sliced_message_size);

      char *tag;
      if (validateTag(&sliced_message, &tag, sliced_message_size)) {
        printf("Tag: %s\n", tag);
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