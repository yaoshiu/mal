#ifndef MAL_TYPE_H
#define MAL_TYPE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum MalType {
  MAL_ATOM_LIST,
  MAL_BOOL,
  MAL_EOF,
  MAL_HASHMAP,
  MAL_INT,
  MAL_KEYWORD,
  MAL_NIL,
  MAL_STRING,
  MAL_SYMBOL,
  MAL_VECTOR,
} MalType;

typedef struct MalAtom {
  MalType type;
  union MalValue {
    int digit;
    char *symbol;
    char *keyword;
    char *string;
    bool boolean;
    struct MalVector *vector;
    struct MalHashmap *hashmap;
    struct MalAtom *children;
  } value;
  struct MalAtom *next;
} MalAtom;

// Free a `MalAtom`
void malatom_free(MalAtom *malatom);

// Create a new `MalAtom`.
//
// The `MalAtom` should be freed with `malatom_free` by the caller.
MalAtom *malatom_new(const MalType type);

typedef struct MalVector {
  int size;
  int capacity;
  struct MalAtom **buffer;
} MalVector;

// Create a new `MalVector`
//
// The `MalVector` should be freed with `malvector_free` by the caller.
MalVector *malvector_new(const int capacity);

// Free a `MalVector`
void malvector_free(MalVector *vector);

// Set the value of a `MalVector` at a given index
//
// Takes ownership of the `MalAtom` passed in. Which means that the caller
// should only use `malvector_free` to free the `MalAtom` passed in.
int malvoctor_set(MalVector *vector, const int index, MalAtom *atom);

// Resize a `MalVector`
int malvector_resize(MalVector *vector, const int capacity);

// Push a `MalAtom` to the end of a `MalVector`
//
// Takes ownership of the `MalAtom` passed in. Which means that the caller
// should only use `malvector_free` to free the `MalAtom` passed in.
int malvector_push(MalVector *vector, MalAtom *atom);

// Pop a `MalAtom` from the end of a `MalVector`. Will also free the poped
// `MalAtom`
int malvector_pop(MalVector *vector);

// Get the value of a `MalVector` at a given index
//
// The `MalAtom` returned should not be freed by the caller.
const MalAtom *malvector_get(const MalVector *vector, const int index);

typedef struct MalHashentry {
  MalAtom *key;
  MalAtom *value;
  int psl;
  struct MalHashentry *next;
} MalHashentry;

typedef struct MalHashmap {
  int capacity;
  int size;
  MalHashentry **buffer;
  MalHashentry *head;
  uint8_t key[16];
} MalHashmap;

// Create a new `MalHashmap`
//
// The `MalHashmap` should be freed with `malhashmap_free` by the caller.
MalHashmap *malhashmap_new(const int capacity);

// Hash function
uint32_t hash(const char *key, const uint8_t seed[16]);

// Free a `MalHashmap`
void malhashmap_free(MalHashmap *hashmap);

// Insert a `MalAtom` into a `MalHashmap` at a given key
//
// Takes ownership of the `MalAtom` passed in. Which means that the caller
// should only use `malhashmap_free` to free the `MalAtom` passed in.
int malhashmap_insert(MalHashmap *hashmap, MalAtom *key, MalAtom *atom);

// Get the value of a `MalHashmap` at a given key
//
// The `MalAtom` returned should not be freed by the caller.
const MalAtom *malhashmap_get(const MalHashmap *hashmap, const MalAtom *key);

// Resize a `MalHashmap`
int malhashmap_resize(MalHashmap *hashmap, const int capacity);

#endif /* MAL_TYPE_H */
