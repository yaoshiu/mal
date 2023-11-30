#include <stdio.h>
#include <stdlib.h>

#include "SipHash/siphash.h"
#include "consts.h"
#include "printer.h"
#include "types.h"

MalAtom *malatom_new(const MalType type) {
  MalAtom *atom = (MalAtom *)malloc(sizeof(MalAtom));
  if (atom == NULL) {
    perror("Failed to allocate memory for atom");
    return NULL;
  }
  atom->type = type;
  atom->next = NULL;
  switch (type) {
  case MAL_SYMBOL:
    atom->value.symbol = NULL;
    break;
  case MAL_STRING:
    atom->value.string = NULL;
    break;
  case MAL_ATOM_LIST:
    atom->value.children = NULL;
    break;
  case MAL_KEYWORD:
    atom->value.keyword = NULL;
    break;
  case MAL_VECTOR:
    atom->value.vector = NULL;
    break;
  case MAL_HASHMAP:
    atom->value.hashmap = NULL;
    break;
  case MAL_BOOL:
  case MAL_EOF:
  case MAL_INT:
  case MAL_NIL:
    break;
  }
  return atom;
}

void malatom_free(MalAtom *atom) {
  if (atom == NULL) {
    return;
  }
  switch (atom->type) {
  case MAL_SYMBOL:
    if (atom->value.symbol == NULL) {
      break;
    }
    free(atom->value.symbol);
    atom->value.symbol = NULL;
    break;
  case MAL_STRING:
    if (atom->value.string == NULL) {
      break;
    }
    free(atom->value.string);
    atom->value.string = NULL;
    break;
  case MAL_ATOM_LIST:
    if (atom->value.children == NULL) {
      break;
    }
    malatom_free(atom->value.children);
    break;
  case MAL_KEYWORD:
    if (atom->value.keyword == NULL) {
      break;
    }
    free(atom->value.keyword);
    atom->value.keyword = NULL;
    break;
  case MAL_VECTOR:
    malvector_free(atom->value.vector);
    atom->value.vector = NULL;
    break;
  case MAL_HASHMAP:
    malhashmap_free(atom->value.hashmap);
    atom->value.hashmap = NULL;
    break;
  case MAL_BOOL:
  case MAL_EOF:
  case MAL_INT:
  case MAL_NIL:
    break;
  }

  if (atom->next != NULL) {
    malatom_free(atom->next);
  }

  free(atom);
}

MalVector *malvector_new(const int capacity) {
  MalVector *vector = (MalVector *)malloc(sizeof(MalVector));
  if (vector == NULL) {
    perror("Failed to allocate memory for vector");
    return NULL;
  }
  vector->capacity = capacity;
  vector->size = 0;
  vector->buffer = (MalAtom **)calloc(capacity, sizeof(MalAtom *));
  if (vector->buffer == NULL) {
    perror("Failed to allocate memory for vector buffer");
    free(vector);
    return NULL;
  }
  return vector;
}

void malvector_free(MalVector *vector) {
  if (vector == NULL) {
    return;
  }
  if (vector->buffer != NULL) {
    for (int i = 0; i < vector->size; i++) {
      malatom_free(vector->buffer[i]);
    }
    free(vector->buffer);
  }
  free(vector);
}

const MalAtom *malvector_get(const MalVector *vector, const int index) {
  if (index < 0 || index >= vector->size) {
    fprintf(stderr, "Index %d out of bounds for vector of size %d\n", index,
            vector->size);

    return NULL;
  }

  return vector->buffer[index];
}

int malvoctor_set(MalVector *vector, const int index, MalAtom *atom) {
  if (index < 0 || index >= vector->size) {
    fprintf(stderr, "Index %d out of bounds for vector of size %d\n", index,
            vector->size);
    malatom_free(atom);
    return 1;
  }

  vector->buffer[index] = atom;
  return 0;
}

int malvector_resize(MalVector *vector, const int capacity) {
  MalAtom **new_buffer =
      (MalAtom **)realloc(vector->buffer, sizeof(MalAtom *) * capacity);
  if (new_buffer == NULL) {
    perror("Failed to resize vector");
    return 1;
  }

  vector->buffer = new_buffer;
  vector->capacity = capacity;
  return 0;
}

int malvector_push(MalVector *vector, MalAtom *atom) {
  if (vector->size >= vector->capacity) {
    if (malvector_resize(vector, vector->capacity * 2)) {
      malatom_free(atom);
      return 1;
    }
  }

  vector->buffer[vector->size++] = atom;
  return 0;
}

int malvector_pop(MalVector *vector) {
  if (vector->size == 0) {
    fprintf(stderr, "Cannot pop from empty vector\n");
    return 1;
  }

  malatom_free(vector->buffer[vector->size - 1]);
  vector->buffer[vector->size - 1] = NULL;
  vector->size--;
  return 0;
}

MalHashentry *malhashentry_new(char *key, void *atom,
                               void (*free_value)(void *)) {
  MalHashentry *entry = (MalHashentry *)malloc(sizeof(MalHashentry));
  if (entry == NULL) {
    perror("Failed to allocate memory for hashmap entry");
    free(key);
    free_value(atom);
    return NULL;
  }
  entry->key = key;
  entry->value = atom;
  entry->psl = 0;
  entry->next = NULL;
  entry->free_value = free_value;
  return entry;
}

void malhashentry_free(MalHashentry *entry) {
  if (entry == NULL) {
    return;
  }
  free(entry->key);
  entry->key = NULL;
  entry->free_value(entry->value);
  entry->value = NULL;
  free(entry);
}

MalHashmap *malhashmap_new(const int capacity) {
  MalHashmap *hashmap = (MalHashmap *)malloc(sizeof(MalHashmap));
  if (hashmap == NULL) {
    perror("Failed to allocate memory for hashmap");
    return NULL;
  }

  hashmap->capacity = capacity;
  hashmap->size = 0;

  for (int i = 0; i < capacity; i++) {
    hashmap->key[i] = (uint8_t)rand();
  }

  hashmap->buffer = (MalHashentry **)calloc(capacity, sizeof(MalHashentry *));
  if (hashmap->buffer == NULL) {
    perror("Failed to allocate memory for hashmap buffer");
    free(hashmap);
    return NULL;
  }

  hashmap->head = NULL;

  return hashmap;
}

void malhashmap_free(MalHashmap *hashmap) {
  if (hashmap == NULL) {
    return;
  }
  if (hashmap->head != NULL) {
    for (MalHashentry *entry = hashmap->head, *next; entry != NULL;
         entry = next) {
      next = entry->next;
      malhashentry_free(entry);
    }
    free(hashmap->buffer);
  }

  free(hashmap);
}

int malhashmap_resize(MalHashmap *hashmap, const int capacity) {
  MalHashentry **old_buffer = hashmap->buffer;
  MalHashentry *old_head = hashmap->head;

  if (hashmap == NULL) {
    fprintf(stderr, "Cannot resize NULL hashmap\n");
    return 1;
  }
  hashmap->capacity = capacity;
  hashmap->size = 0;
  hashmap->buffer = (MalHashentry **)calloc(capacity, sizeof(MalHashentry *));
  if (hashmap->buffer == NULL) {
    perror("Failed to allocate memory for hashmap buffer");
    hashmap->buffer = old_buffer;
    return 1;
  }
  hashmap->head = NULL;
  for (MalHashentry *current = old_head, *next; current != NULL;
       current = next) {
    // TODO: Check for errors
    malhashmap_insert(hashmap, current->key, current->value,
                      current->free_value);

    next = current->next;

    free(current);
  }
  free(old_buffer);

  return 0;
}

uint32_t hash(const char *key, const uint8_t seed[16]) {
  uint8_t out[8];
  siphash((uint8_t *)key, strlen(key), seed, out, 8);

  return *(uint32_t *)out;
}

int malhashmap_insert(MalHashmap *hashmap, char *key, void *value,
                      void (*free_value)(void *)) {
  if (hashmap->size >= hashmap->capacity * LOAD_FACTOR) {
    if (malhashmap_resize(hashmap, hashmap->capacity * 2)) {
      free(key);
      free_value(value);
      return 1;
    }
  }

  MalHashentry *entry = malhashentry_new(key, value, free_value);
  if (entry == NULL) {
    return 1;
  }
  key = NULL;
  value = NULL;

  MalHashentry *tmp = entry;

  uint32_t index = hash(entry->key, hashmap->key) % hashmap->capacity;
  while (hashmap->buffer[index] != NULL) {
    char *index_key = hashmap->buffer[index]->key;
    if (strcmp(index_key, entry->key) == 0) {
      hashmap->buffer[index]->value = entry->value;
      free(entry->key);
      free(entry);
      return 0;
    }

    if (hashmap->buffer[index]->psl < entry->psl) {
      MalHashentry *temp = hashmap->buffer[index];
      hashmap->buffer[index] = entry;
      entry = temp;
    }
    index = (index + 1) % hashmap->capacity;
    entry->psl++;
  }

  if (hashmap->head == NULL) {
    hashmap->head = tmp;
  } else {
    tmp->next = hashmap->head;
    hashmap->head = tmp;
  }

  hashmap->buffer[index] = entry;
  hashmap->size++;
  return 0;
}

const void *malhashmap_get(const MalHashmap *hashmap, const char *key) {
  uint32_t index = hash(key, hashmap->key) % hashmap->capacity;
  while (hashmap->buffer[index] != NULL) {
    char *index_key = hashmap->buffer[index]->key;
    if (strcmp(index_key, key) == 0) {
      return hashmap->buffer[index]->value;
    }

    index = (index + 1) % hashmap->capacity;
  }

  return NULL;
}
