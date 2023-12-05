#include <stdio.h>
#include <stdlib.h>

#include "consts.h"
#include "env.h"

Env *env_new(const Env *outer, const int capacity) {
  Env *env = (Env *)malloc(sizeof(Env));
  if (env == NULL) {
    perror("Cannot allocate memory for Env");
    return NULL;
  }
  env->data = malhashmap_new(capacity);
  if (env->data == NULL) {
    perror("Cannot allocate memory for Env");
    free(env);
    return NULL;
  }
  env->outer = outer;
  return env;
}

void env_free(Env *env) {
  if (env == NULL) {
    return;
  }
  malhashmap_free(env->data);
  free(env);
}

const MalAtom *env_get(const Env *env, const MalAtom *key) {
  if (env == NULL) {
    fprintf(stderr, "Cannot get value from NULL Env\n");
    return NULL;
  }

  if (key == NULL) {
    fprintf(stderr, "Cannot get value from Env with NULL key\n");
    return NULL;
  }

  env = env_find(env, key);
  if (env == NULL) {
    return NULL;
  }

  return malhashmap_get(env->data, key);
}

const Env *env_find(const Env *env, const MalAtom *key) {
  if (env == NULL) {
    fprintf(stderr, "Cannot find value from NULL Env\n");
    return NULL;
  }

  if (key == NULL) {
    fprintf(stderr, "Cannot find value from Env with NULL key\n");
    return NULL;
  }

  while (env != NULL) {
    if (malhashmap_get(env->data, key) != NULL) {
      return env;
    }
    env = env->outer;
  }
  return NULL;
}

int env_set(Env *env, MalAtom *key, MalAtom *value) {
  if (key == NULL) {
    if (value != NULL) {
      malatom_free(value);
    }
    fprintf(stderr, "Cannot set value in Env with NULL key\n");
    return 1;
  }

  if (value == NULL) {
    malatom_free(key);
    fprintf(stderr, "Cannot set NULL value in Env\n");
    return 1;
  }

  if (env == NULL) {
    malatom_free(key);
    malatom_free(value);
    fprintf(stderr, "Cannot set value in NULL Env\n");
    return 1;
  }

  return malhashmap_insert(env->data, key, value);
}
