#include <stdio.h>
#include <stdlib.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "consts.h"
#include "printer.h"
#include "reader.h"
#include "types.h"

// Read a str and return a `MalAtom`
//
// The returned `MalAtom` should be freed by the caller.
MalAtom *READ(const char *str) { return read_str(str); }

// Evaluates the given `MalAtom` in the provided environment.
//
// Takes ownership of the input `atom` - responsibility for freeing is
// transferred to this function. The caller should not free atom after
// passing to `EVAL`.
//
// Also takes a reference to the `repl_env` map. Responsibility for freeing
// `repl_env` remains with the caller.
//
// Returns a new `MalAtom` representing the evaluated result. The caller is
// responsible for freeing the returned atom by calling `malatom_free()`
// when no longer needed.
MalAtom *EVAL(MalAtom *atom, const MalHashmap *repl_env);

// Evaluates the given AST in the provided environment map.
//
// The responsibility for freeing the input `ast` is transferred to this
// function. The caller should not free ast after passing it in.
//
// Returns a new `MalAtom` representing the evaluation result. The returned
// atom is allocated within this function and the caller is responsible
// for freeing it with `malatom_free()` when no longer needed.
//
// The `repl_env` map is borrowed during execution. Responsibility for
// freeing `repl_env` remains with the caller.
MalAtom *eval_ast(MalAtom *ast, const MalHashmap *repl_env) {
  if (ast == NULL) {
    return NULL;
  }

  switch (ast->type) {
  case MAL_SYMBOL: {
    const MalAtom *value =
        (const MalAtom *)malhashmap_get(repl_env, ast->value.symbol);
    malatom_free(ast);
    if (value == NULL) {
      fprintf(stderr, "Symbol not found: %s\n", ast->value.symbol);
      return NULL;
    }
    MalAtom *copy = malatom_new(value->type);
    if (copy == NULL) {
      perror("Failed to allocate memory for atom");
      return NULL;
    }
    copy->value = value->value;
    return copy;
  }

  case MAL_ATOM_LIST: {
    MalAtom *prev = ast;
    for (MalAtom *it = ast->value.children, *next = NULL; it != NULL;
         it = next) {
      next = it->next;

      // Delete the current element from the list
      it->next = NULL;
      if (prev == ast) {
        prev->value.children = next;
      } else {
        prev->next = next;
      }

      it = EVAL(it, repl_env);
      if (it == NULL) {
        malatom_free(ast);
        return NULL;
      }

      // Insert the evaluated element back into the list
      it->next = next;
      if (prev == ast) {
        prev->value.children = it;
      } else {
        prev->next = it;
      }

      prev = it;
    }
    return ast;
  }

  case MAL_VECTOR:
    for (int i = 0; i < ast->value.vector->size; i++) {
      MalAtom *value = EVAL(ast->value.vector->buffer[i], repl_env);
      if (value == NULL) {
        malatom_free(ast);
        return NULL;
      }
      ast->value.vector->buffer[i] = value;
    }
    return ast;

  case MAL_HASHMAP:
    for (MalHashentry *it = ast->value.hashmap->head; it != NULL;
         it = it->next) {
      MalAtom *value = EVAL((MalAtom *)it->value, repl_env);
      if (value == NULL) {
        malatom_free(ast);
        return NULL;
      }
      it->value = value;
    }
    return ast;

  default:
    return ast;
  }
}

MalAtom *EVAL(MalAtom *atom, const MalHashmap *repl_env) {
  if (atom == NULL) {
    return NULL;
  }

  if (atom->type == MAL_ATOM_LIST) {
    MalAtom *list = eval_ast(atom, repl_env);
    atom = NULL;
    if (list == NULL) {
      return NULL;
    }
    if (list->value.children == NULL) {
      return list;
    }
    MalAtom *first = list->value.children;
    if (first->type != MAL_FUNCTION) {
      fprintf(stderr, "First element is not a function\n");
      malatom_free(list);
      return NULL;
    }

    MalAtom *(*fn)(const MalAtom *) =
        (MalAtom * (*)(const MalAtom *)) first->value.function;

    MalAtom *result = fn(first->next);
    malatom_free(list);
    return result;
  }

  return eval_ast(atom, repl_env);
}

// Print a MalAtom and return a string
//
// The returned string should be freed by the caller.
char *PRINT(MalAtom *atom) {
  if (atom == NULL) {
    return NULL;
  }
  char *str = pr_str(atom, true);
  malatom_free(atom);
  return str;
}
// Add two or more integers
//
// Returns a `MalAtom` that should be freed by the caller.
MalAtom *plus(const MalAtom *args) {
  if (args == NULL) {
    return NULL;
  }
  int sum = 0;
  for (const MalAtom *it = args; it != NULL; it = it->next) {
    if (it->type != MAL_INT) {
      fprintf(stderr, "Expected a list of integers\n");
      return NULL;
    }
    sum += it->value.integer;
  }
  MalAtom *atom = malatom_new(MAL_INT);
  if (atom == NULL) {
    perror("Failed to allocate memory for atom");
    return NULL;
  }
  atom->value.integer = sum;
  return atom;
}

// Subtract two or more integers
//
// Returns a `MalAtom` that should be freed by the caller.
MalAtom *minus(const MalAtom *args) {
  if (args == NULL) {
    fprintf(stderr, "Wrong number of arguments\n");
    return NULL;
  }
  int sum = 0;
  for (const MalAtom *it = args; it != NULL; it = it->next) {
    if (it->type != MAL_INT) {
      fprintf(stderr, "Expected a list of integers\n");
      return NULL;
    }
    if (it == args->next) {
      sum = -sum;
    }
    sum -= it->value.integer;
  }
  MalAtom *atom = malatom_new(MAL_INT);
  if (atom == NULL) {
    perror("Failed to allocate memory for atom");
    return NULL;
  }
  atom->value.integer = sum;
  return atom;
}

// Multiply two or more integers
//
// Returns a `MalAtom` that should be freed by the caller.
MalAtom *multiply(const MalAtom *args) {
  if (args == NULL) {
    return NULL;
  }
  int product = 1;
  for (const MalAtom *it = args; it != NULL; it = it->next) {
    if (it->type != MAL_INT) {
      fprintf(stderr, "Expected a list of integers\n");
      return NULL;
    }
    product *= it->value.integer;
  }
  MalAtom *atom = malatom_new(MAL_INT);
  if (atom == NULL) {
    perror("Failed to allocate memory for atom");
    return NULL;
  }
  atom->value.integer = product;
  return atom;
}

// Divide two or more integers
//
// Returns a `MalAtom` that should be freed by the caller.
MalAtom *divide(const MalAtom *args) {
  if (args == NULL) {
    fprintf(stderr, "Wrong number of arguments\n");
    return NULL;
  }
  int product = 1;
  for (const MalAtom *it = args; it != NULL; it = it->next) {
    if (it->type != MAL_INT) {
      fprintf(stderr, "Expected a list of integers\n");
      return NULL;
    }
    if (it == args->next) {
      product = args->value.integer;
    }
    product /= it->value.integer;
  }
  MalAtom *atom = malatom_new(MAL_INT);
  if (atom == NULL) {
    perror("Failed to allocate memory for atom");
    return NULL;
  }
  atom->value.integer = product;
  return atom;
}

// Read a str, evaluate it, print the result and return a string
//
// The returned string should be freed by the caller.
char *rep(const char *str) {
  MalHashmap *repl_env = malhashmap_new(DEFAULT_CONTAINER_CAPACITY);

  char *plus_str = strdup("+");
  MalAtom *plus_atom = malatom_new(MAL_FUNCTION);
  plus_atom->value.function = (void *(*)(void *))plus;
  malhashmap_insert(repl_env, plus_str, plus_atom,
                    (void (*)(void *))malatom_free);
  plus_str = NULL;
  plus_atom = NULL;

  char *minus_str = strdup("-");
  MalAtom *minus_atom = malatom_new(MAL_FUNCTION);
  minus_atom->value.function = (void *(*)(void *))minus;
  malhashmap_insert(repl_env, minus_str, minus_atom,
                    (void (*)(void *))malatom_free);
  minus_str = NULL;
  minus_atom = NULL;

  char *multiply_str = strdup("*");
  MalAtom *multiply_atom = malatom_new(MAL_FUNCTION);
  multiply_atom->value.function = (void *(*)(void *))multiply;
  malhashmap_insert(repl_env, multiply_str, multiply_atom,
                    (void (*)(void *))malatom_free);
  multiply_str = NULL;
  multiply_atom = NULL;

  char *divide_str = strdup("/");
  MalAtom *divide_atom = malatom_new(MAL_FUNCTION);
  divide_atom->value.function = (void *(*)(void *))divide;
  malhashmap_insert(repl_env, divide_str, divide_atom,
                    (void (*)(void *))malatom_free);
  divide_str = NULL;
  divide_atom = NULL;

  char *result = PRINT(EVAL(READ(str), repl_env));
  malhashmap_free(repl_env);
  return result;
}

int main(int argc, char **argv) {
  if (regex_compile()) {
    return 1;
  }

  while (1) {
    const char *input = readline(PROMPT);
    if (input == NULL) {
      break;
    }
    add_history(input);
    const char *output = rep(input);
    free((void *)input);
    if (output == NULL) {
      continue;
    }
    printf("%s\n", output);
    if (output[0] == '\0') {
      free((void *)output);
      break;
    }
    free((void *)output);
  }
  regex_free();

  return 0;
}
