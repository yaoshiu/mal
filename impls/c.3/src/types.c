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
  case MAL_FUNCTION:
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
    for (MalAtom *child = atom->value.children, *next; child != NULL;
         child = next) {
      next = child->next;
      malatom_free(child);
    }
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
  case MAL_FUNCTION:
    break;
  }

  free(atom);
}

MalAtom *malatom_copy(const MalAtom *atom) {
  if (atom == NULL) {
    return NULL;
  }
  MalAtom *copy = malatom_new(atom->type);
  if (copy == NULL) {
    return NULL;
  }
  switch (atom->type) {
  case MAL_SYMBOL:
    copy->value.symbol = strdup(atom->value.symbol);
    if (copy->value.symbol == NULL) {
      perror("Failed to allocate memory for symbol copy");
      malatom_free(copy);
      return NULL;
    }
    break;
  case MAL_STRING:
    copy->value.string = strdup(atom->value.string);
    if (copy->value.string == NULL) {
      perror("Failed to allocate memory for string copy");
      malatom_free(copy);
      return NULL;
    }
    break;
  case MAL_ATOM_LIST:
    copy->value.children = NULL;
    for (MalAtom *child = atom->value.children, *prev = NULL; child != NULL;
         child = child->next) {
      MalAtom *child_copy = malatom_copy(child);
      if (child_copy == NULL) {
        malatom_free(copy);
        return NULL;
      }
      if (prev == NULL) {
        copy->value.children = child_copy;
      } else {
        prev->next = child_copy;
      }
      prev = child_copy;
    }
    break;
  case MAL_KEYWORD:
    copy->value.keyword = strdup(atom->value.keyword);
    if (copy->value.keyword == NULL) {
      perror("Failed to allocate memory for keyword copy");
      malatom_free(copy);
      return NULL;
    }
    break;
  case MAL_VECTOR:
    copy->value.vector = malvector_new(atom->value.vector->capacity);
    if (copy->value.vector == NULL) {
      malatom_free(copy);
      return NULL;
    }
    for (int i = 0; i < atom->value.vector->size; i++) {
      MalAtom *vector_copy = malatom_copy(atom->value.vector->buffer[i]);
      if (vector_copy == NULL) {
        malatom_free(copy);
        return NULL;
      }
      if (malvector_push(copy->value.vector, vector_copy)) {
        malatom_free(copy);
        return NULL;
      }
    }
    break;
  case MAL_HASHMAP:
    copy->value.hashmap = malhashmap_new(atom->value.hashmap->capacity);
    if (copy->value.hashmap == NULL) {
      malatom_free(copy);
      return NULL;
    }
    for (MalHashentry *entry = atom->value.hashmap->head; entry != NULL;
         entry = entry->next) {
      if (malhashmap_insert(copy->value.hashmap, malatom_copy(entry->key),
                            malatom_copy(entry->value))) {
        malatom_free(copy);
        return NULL;
      }
    }
    break;
  case MAL_BOOL:
    copy->value.boolean = atom->value.boolean;
    break;
  case MAL_EOF:
    break;
  case MAL_INT:
    copy->value.integer = atom->value.integer;
    break;
  case MAL_NIL:
    break;
  case MAL_FUNCTION:
    copy->value.function = atom->value.function;
    break;
  }

  return copy;
}

bool malatom_equal(const MalAtom *a, const MalAtom *b) {
  if (a == NULL || b == NULL) {
    return false;
  }
  if (a->type != b->type) {
    return false;
  }
  switch (a->type) {
  case MAL_SYMBOL:
    return strcmp(a->value.symbol, b->value.symbol) == 0;
  case MAL_STRING:
    return strcmp(a->value.string, b->value.string) == 0;
  case MAL_ATOM_LIST:
    for (MalAtom *a_child = a->value.children, *b_child = b->value.children;
         a_child != NULL && b_child != NULL;
         a_child = a_child->next, b_child = b_child->next) {
      if (!malatom_equal(a_child, b_child)) {
        return false;
      }
    }
    return true;
  case MAL_KEYWORD:
    return strcmp(a->value.keyword, b->value.keyword) == 0;
  case MAL_VECTOR:
    if (a->value.vector->size != b->value.vector->size) {
      return false;
    }
    for (int i = 0; i < a->value.vector->size; i++) {
      if (!malatom_equal(a->value.vector->buffer[i],
                         b->value.vector->buffer[i])) {
        return false;
      }
    }
    return true;
  case MAL_HASHMAP: {
    MalAtom *a_copy = malatom_copy(a);
    if (a_copy == NULL) {
      return false;
    }
    for (MalHashentry *entry = a_copy->value.hashmap->head; entry != NULL;
         entry = entry->next) {
      const MalAtom *b_value = malhashmap_get(b->value.hashmap, entry->key);
      if (b_value == NULL || !malatom_equal(entry->value, b_value)) {
        malatom_free(a_copy);
        return false;
      }
      entry->value = NULL;
    }
    for (MalHashentry *entry = a_copy->value.hashmap->head; entry != NULL;
         entry = entry->next) {
      if (entry->value != NULL) {
        malatom_free(a_copy);
        return false;
      }
    }
    malatom_free(a_copy);
    return true;
  }
  case MAL_BOOL:
    return a->value.boolean == b->value.boolean;
  case MAL_EOF:
    return true;
  case MAL_INT:
    return a->value.integer == b->value.integer;
  case MAL_NIL:
    return true;
  case MAL_FUNCTION:
    return a->value.function == b->value.function;
  }
  return false;
}

MalVector *malvector_new(const int capacity) {
  if (capacity <= 0) {
    fprintf(stderr, "Cannot create vector with capacity %d\n", capacity);
    return NULL;
  }

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

int malvector_set(MalVector *vector, const int index, MalAtom *atom) {
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

MalHashentry *malhashentry_new(MalAtom *key, MalAtom *value) {
  if (key == NULL) {
    if (value != NULL) {
      malatom_free(value);
    }
    fprintf(stderr, "Cannot create hashmap entry with NULL key or value\n");
    return NULL;
  }

  if (value == NULL) {
    malatom_free(key);
    fprintf(stderr, "Cannot create hashmap entry with NULL key or value\n");
    return NULL;
  }

  MalHashentry *entry = (MalHashentry *)malloc(sizeof(MalHashentry));
  if (entry == NULL) {
    perror("Failed to allocate memory for hashmap entry");
    malatom_free(key);
    malatom_free(value);
    return NULL;
  }
  entry->key = key;
  entry->value = value;
  entry->psl = 0;
  entry->next = NULL;
  return entry;
}

void malhashentry_free(MalHashentry *entry) {
  if (entry == NULL) {
    return;
  }
  malatom_free(entry->key);
  entry->key = NULL;
  malatom_free(entry->value);
  entry->value = NULL;
  free(entry);
}

MalHashmap *malhashmap_new(const int capacity) {
  if (capacity <= 0) {
    fprintf(stderr, "Cannot create hashmap with capacity %d\n", capacity);
    return NULL;
  }

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

  int state_code = 0;
  for (MalHashentry *current = old_head, *next; current != NULL;
       current = next) {
    if (malhashmap_insert(hashmap, current->key, current->value)) {
      state_code = 1;
    }

    next = current->next;

    free(current);
  }
  free(old_buffer);

  return state_code;
}

uint32_t hash(const char *key, const uint8_t seed[16]) {
  uint8_t out[8];
  siphash((uint8_t *)key, strlen(key), seed, out, 8);

  return *(uint32_t *)out;
}

int malhashmap_insert(MalHashmap *hashmap, MalAtom *key, MalAtom *value) {
  if (hashmap->size >= hashmap->capacity * LOAD_FACTOR) {
    if (malhashmap_resize(hashmap, hashmap->capacity * 2)) {
      malatom_free(key);
      malatom_free(value);
      return 1;
    }
  }

  MalHashentry *entry = malhashentry_new(key, value);
  if (entry == NULL) {
    return 1;
  }
  key = NULL;
  value = NULL;

  MalHashentry *tmp = entry;

  char *key_str = pr_str(entry->key, false);
  uint32_t index = hash(key_str, hashmap->key) % hashmap->capacity;
  free(key_str);
  while (hashmap->buffer[index] != NULL) {
    if (malatom_equal(hashmap->buffer[index]->key, entry->key)) {
      hashmap->buffer[index]->value = entry->value;
      malatom_free(entry->key);
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

const MalAtom *malhashmap_get(const MalHashmap *hashmap, const MalAtom *key) {
  if (hashmap == NULL) {
    fprintf(stderr, "Cannot get value from NULL hashmap\n");
    return NULL;
  }
  char *key_str = pr_str(key, false);
  uint32_t index = hash(key_str, hashmap->key) % hashmap->capacity;
  free(key_str);
  while (hashmap->buffer[index] != NULL) {
    if (malatom_equal(hashmap->buffer[index]->key, key)) {
      return hashmap->buffer[index]->value;
    }

    index = (index + 1) % hashmap->capacity;
  }

  return NULL;
}
