#ifndef MAL_ENV_H
#define MAL_ENV_H

#include "types.h"

typedef struct Env {
  MalHashmap *data;
  const struct Env *outer;
} Env;

/**
 * Allocates memory and initializes a new environment.
 *
 * Parameters:
 * - outer: The outer env to search if symbol not found in this env. Caller
 * retains ownership.
 * - capacity: Initial capacity of the hashmap for this env.
 *
 * Return:
 * - New Env struct. Caller takes ownership.
 *    Must be freed later with env_free().
 */
Env *env_new(const Env *outer, const int capacity);

/**
 * Frees memory allocated for an environment.
 *
 * Parameters:
 * - env: Env to free. Takes ownership from caller.
 */
void env_free(Env *env);

/**
 * Gets a value from the environment or any outer environments.
 *
 * Parameters:
 * - env: Env struct to search. Caller retains ownership.
 * - key: Key for the value to get. Caller retains ownership.
 *
 * Return:
 * - Value for the key if found, NULL if not found.
 *   Caller does NOT take ownership.
 */
const MalAtom *env_get(const Env *env, const MalAtom *key);

/**
 * Finds the env that contains the given key, searching outwards.
 *
 * Parameters:
 * - env: Env struct to search. Caller retains ownership.
 * - key: Key to find env for. Caller retains ownership.
 *
 * Return:
 * - Env that contains the key if found, NULL if not found.
 *   Caller does NOT take ownership.
 */
const Env *env_find(const Env *env, const MalAtom *key);

/**
 * Inserts a key-value pair into the env.
 *
 * Parameters:
 * - env: Env struct to insert into. Caller retains ownership.
 * - key: Key for insertion. Method takes ownership.
 * - value: Value for insertion. Method takes ownership.
 * - free_value: Function to free value when removed from map.
 *
 * Return:
 * - 0 if success, 1 if error.
 */
int env_set(Env *env, MalAtom *key, MalAtom *value);

#endif
