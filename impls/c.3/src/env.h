#ifndef MAL_ENV_H
#define MAL_ENV_H

#include "types.h"

typedef struct Env {
  MalHashmap *data;
  const struct Env *outer;
} Env;

// Create a new environment with the given outer environment.
//
// Does not take ownership of the outer environment.
// The outer environment will not be freed when the new environment is freed.
Env *env_new(const Env *outer, const int capacity);

// Free the given environment.
void env_free(Env *env);

// Get the value of the given key in the environment.
//
// Returns NULL if the key is not found.
const void *env_get(const Env *env, const char *key);

// Find the environment that contains the given key.
const Env *env_find(const Env *env, const char *key);

// Set the value of the given key in the environment.
//
// This function takes ownership of the `key` and `value`.
// The key and value will be freed when the environment is freed.
int env_set(Env *env, char *key, void *val, void (*free_val)(void *));

#endif
