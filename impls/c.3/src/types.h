#ifndef MAL_TYPE_H
#define MAL_TYPE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum MalType {
  MAL_FUNCTION,
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
    int integer;
    char *symbol;
    char *keyword;
    char *string;
    bool boolean;
    void *(*function)(void *);
    struct MalVector *vector;
    struct MalHashmap *hashmap;
    struct MalAtom *children;
  } value;
  struct MalAtom *next;
} MalAtom;

/**
 * Frees memory for a MalAtom.
 *
 * Paramaters:
 * - atom: MalAtom to free. Takes ownership from caller.
 */
void malatom_free(MalAtom *atom);

/**
 * Allocates and initializes a new MalAtom.
 *
 * Parameters:
 * - type: Type of MalAtom to create.
 *
 * Returns:
 * - New MalAtom struct. Caller takes ownership.
 *   Must be freed later with malatom_free().
 */
MalAtom *malatom_new(const MalType type);

/**
 * Allocates and initializes a new MalAtom with a given value.
 *
 * Parameters:
 * - atom: MalAtom to set value of. Caller retains ownership.
 *
 * Returns:
 * - New MalAtom struct. Caller takes ownership.
 *   Must be freed later with malatom_free().
 */
MalAtom *malatom_copy(const MalAtom *atom);

/**
 * Compare two MalAtoms for equality.
 *
 * Parameters:
 * - a: First MalAtom to compare. Caller retains ownership.
 * - b: Second MalAtom to compare. Caller retains ownership.
 *
 * Returns:
 * - True if MalAtoms are equal, false otherwise.
 */
bool malatom_equal(const MalAtom *a, const MalAtom *b);

typedef struct MalVector {
  int size;
  int capacity;
  struct MalAtom **buffer;
} MalVector;

/**
 * Allocates and initializes a new MalVector.
 *
 * Parameters:
 * - capacity: Initial capacity of vector.
 *
 * Returns:
 * - New MalVector struct. Caller takes ownership.
 *   Must be freed later with malvector_free().
 */
MalVector *malvector_new(const int capacity);

/**
 * Frees memory for a MalVector.
 *
 * Paramaters:
 * - vector: MalVector to free. Takes ownership from caller.
 */
void malvector_free(MalVector *vector);

/**
 * Set the value of a MalVector at a given index.
 *
 * Parameters:
 * - vector: MalVector to set value of. Caller retains ownership.
 * - index: Index to set value at.
 * - atom: MalAtom to set value to. Takes ownership from caller.
 *
 * Returns:
 * - 0 on success, 1 on failure to set value.
 */
int malvector_set(MalVector *vector, const int index, MalAtom *atom);

/**
 * Resize a MalVector.
 *
 * Parameters:
 * - vector: MalVector to resize. Caller retains ownership.
 * - capacity: New capacity of vector.
 *
 * Returns:
 * - 0 on success, 1 on failure to resize.
 */
int malvector_resize(MalVector *vector, const int capacity);

/**
 * Push a MalAtom onto the end of a MalVector.
 *
 * Parameters:
 * - vector: MalVector to push value onto. Caller retains ownership.
 * - atom: MalAtom to push onto vector. Takes ownership from caller.
 *
 * Returns:
 * - 0 on success, 1 on failure to push value.
 */
int malvector_push(MalVector *vector, MalAtom *atom);

/**
 * Pop a MalAtom from the end of a MalVector.
 *
 * Parameters:
 * - vector: MalVector to pop value from. Caller retains ownership.
 *
 * Returns:
 * - 0 on success, 1 on failure to pop value.
 */
int malvector_pop(MalVector *vector);

/**
 * Get the value of a MalVector at a given index.
 *
 * Parameters:
 * - vector: MalVector to get value from. Caller retains ownership.
 * - index: Index to get value from.
 *
 * Returns:
 * - MalAtom at index. Caller does NOT take ownership.
 */
const MalAtom *malvector_get(const MalVector *vector, const int index);

typedef struct MalHashentry {
  MalAtom *key;
  MalAtom *value;
  int psl;
  struct MalHashentry *next;
} MalHashentry;

/**
 * Allocates and initializes a new MalHashentry.
 *
 * Parameters:
 * - key: Key of MalHashentry. Takes ownership from caller.
 * - value: Value of MalHashentry. Takes ownership from caller.
 *
 * Returns:
 * - New MalHashentry struct. Caller takes ownership.
 *   Must be freed later with malhashentry_free().
 */
MalHashentry *malhashentry_new(MalAtom *key, MalAtom *value);

/**
 * Frees memory for a MalHashentry.
 */
void malhashentry_free(MalHashentry *hashentry);

typedef struct MalHashmap {
  int capacity;
  int size;
  MalHashentry **buffer;
  MalHashentry *head;
  uint8_t key[16];
} MalHashmap;

/**
 * Allocates and initializes a new MalHashmap.
 *
 * Parameters:
 * - capacity: Initial capacity of hashmap.
 *
 * Returns:
 * - New MalHashmap struct. Caller takes ownership.
 *   Must be freed later with malhashmap_free().
 */
MalHashmap *malhashmap_new(const int capacity);

/**
 * Hash a string.
 *
 * Parameters:
 * - key: String to hash. Caller retains ownership.
 * - seed: Seed to hash with.
 *
 * Returns:
 * - Hash of string.
 */
uint32_t hash(const char *key, const uint8_t seed[16]);

/**
 * Frees memory for a MalHashmap.
 */
void malhashmap_free(MalHashmap *hashmap);

/**
 * Insert a value into a MalHashmap at a given key.
 *
 * Parameters:
 * - hashmap: MalHashmap to insert value into. Caller retains ownership.
 * - key: Key to insert value at. Takes ownership from caller.
 * - value: Value to insert. Takes ownership from caller.
 *
 * Returns:
 * - 0 on success, 1 on failure to insert value.
 */
int malhashmap_insert(MalHashmap *hashmap, MalAtom *key, MalAtom *value);

/**
 * Get a value from a MalHashmap at a given key.
 *
 * Parameters:
 * - hashmap: MalHashmap to get value from. Caller retains ownership.
 * - key: Key to get value from. Caller retains ownership.
 *
 * Returns:
 * - Value at key. Caller does NOT take ownership.
 */
const MalAtom *malhashmap_get(const MalHashmap *hashmap, const MalAtom *key);

/**
 * Resize a MalHashmap.
 *
 * Parameters:
 * - hashmap: MalHashmap to resize. Caller retains ownership.
 * - capacity: New capacity of hashmap.
 *
 * Returns:
 * - 0 on success, 1 on failure to resize.
 */
int malhashmap_resize(MalHashmap *hashmap, const int capacity);

#endif /* MAL_TYPE_H */
