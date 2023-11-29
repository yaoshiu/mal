#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "consts.h"
#include "printer.h"

char *pr_str(const MalAtom *atom, const bool print_readably) {
  char *str;
  if (atom == NULL) {
    return NULL;
  }

  switch (atom->type) {
  case MAL_SYMBOL:
    str = strdup(atom->value.symbol);
    break;

  case MAL_INT:
    str = (char *)calloc(BUFFER_SIZE, sizeof(char));
    if (str == NULL) {
      perror("Failed to allocate memory for int");
      break;
    }
    sprintf(str, "%d", atom->value.digit);
    break;

  case MAL_EOF:
    str = strdup("\0");
    break;

  case MAL_STRING: {
    str = (char *)calloc(BUFFER_SIZE, sizeof(char));
    if (str == NULL) {
      perror("Failed to allocate memory for string");
      break;
    }
    strcpy(str, "\"");
    int len = strlen(atom->value.string);
    if (print_readably) {
      for (int i = 0; i < len; i++) {
        switch (atom->value.string[i]) {
        case '\n':
          strcat(str, "\\n");
          break;
        case '\t':
          strcat(str, "\\t");
          break;
        case '\r':
          strcat(str, "\\r");
          break;
        case '"':
          strcat(str, "\\\"");
          break;
        case '\\':
          strcat(str, "\\\\");
          break;
        default:
          strncat(str, &atom->value.string[i], 1);
        }
      }
    } else {
      strcat(str, atom->value.string);
    }
    strcat(str, "\"");
    break;
  }

  case MAL_ATOM_LIST:
    str = (char *)calloc(BUFFER_SIZE, sizeof(char));
    if (str == NULL) {
      perror("Failed to allocate memory for list");
      break;
    }
    strcpy(str, "(");
    for (MalAtom *ptr = atom->value.children; ptr != NULL; ptr = ptr->next) {
      char *tmp = pr_str(ptr, print_readably);
      strcat(str, tmp);
      free(tmp);
      if (ptr->next != NULL) {
        strcat(str, " ");
      }
    }
    strcat(str, ")");
    break;
  case MAL_BOOL:
    if (atom->value.boolean) {
      str = strdup("true");
    } else {
      str = strdup("false");
    }
    break;
  case MAL_KEYWORD:
    str = (char *)calloc(BUFFER_SIZE, sizeof(char));
    if (str == NULL) {
      perror("Failed to allocate memory for keyword");
      break;
    }
    strcpy(str, ":");
    strcat(str, atom->value.keyword);
    break;
  case MAL_NIL:
    str = strdup("nil");
    break;
  case MAL_VECTOR:
    str = (char *)calloc(BUFFER_SIZE, sizeof(char));
    if (str == NULL) {
      perror("Failed to allocate memory for vector");
      break;
    }
    strcpy(str, "[");
    for (int i = 0; i < atom->value.vector->size; i++) {
      char *tmp = pr_str(atom->value.vector->buffer[i], print_readably);
      strcat(str, tmp);
      free(tmp);
      if (i != atom->value.vector->size - 1) {
        strcat(str, " ");
      }
    }
    strcat(str, "]");
    break;
  case MAL_HASHMAP:
    str = (char *)calloc(BUFFER_SIZE, sizeof(char));
    if (str == NULL) {
      perror("Failed to allocate memory for hashmap");
      break;
    }
    strcpy(str, "{");
    for (MalHashentry *entry = atom->value.hashmap->head; entry != NULL;
         entry = entry->next) {
      char *key = pr_str(entry->key, print_readably);
      char *val = pr_str(entry->value, print_readably);
      strcat(str, key);
      strcat(str, " ");
      strcat(str, val);
      free(key);
      free(val);
      if (entry->next != NULL) {
        strcat(str, " ");
      }
    }
    strcat(str, "}");
    break;
  }

  return str;
}
