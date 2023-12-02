#include <stdio.h>
#include <stdlib.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "consts.h"
#include "env.h"
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
MalAtom *EVAL(MalAtom *atom, Env *repl_env);

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
MalAtom *eval_ast(MalAtom *ast, Env *repl_env) {
  if (ast == NULL) {
    return NULL;
  }

  switch (ast->type) {
  case MAL_SYMBOL: {
    const MalAtom *value =
        (const MalAtom *)env_get(repl_env, ast->value.symbol);
    malatom_free(ast);
    if (value == NULL) {
      return NULL;
    }
    MalAtom *copy = malatom_copy(value);
    if (copy == NULL) {
      perror("Failed to allocate memory for atom");
      return NULL;
    }
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

// Evaluates a `def!` special form.
//
// The responsibility for freeing the input `ast` is transferred to this
// function. The caller should not free ast after passing it in.
// The `repl_env` map is borrowed during execution. Responsibility for
// freeing `repl_env` remains with the caller.
//
// Returns a new `MalAtom` representing the evaluation result. The returned
// atom is allocated within this function and the caller is responsible
// for freeing it with `malatom_free()` when no longer needed.
MalAtom *eval_def(MalAtom *ast, Env *repl_env) {
  if (ast == NULL) {
    return NULL;
  }

  if (ast->type != MAL_ATOM_LIST) {
    fprintf(stderr, "Expected a list\n");
    malatom_free(ast);
    return NULL;
  }

  MalAtom *symbol = ast->value.children;
  if (symbol == NULL || symbol->type != MAL_SYMBOL ||
      strcmp(symbol->value.symbol, "def!") != 0) {
    fprintf(stderr, "Expected a symbol 'def!'\n");
    malatom_free(ast);
    return NULL;
  }

  MalAtom *key = symbol->next;
  if (key == NULL || key->type != MAL_SYMBOL) {
    fprintf(stderr, "Expected a symbol\n");
    malatom_free(ast);
    return NULL;
  }

  MalAtom *value = key->next;
  if (value == NULL || value->next != NULL) {
    fprintf(stderr, "Expected a value\n");
    malatom_free(ast);
    return NULL;
  }

  // Delete the current element from the list
  key->next = NULL;
  value = EVAL(value, repl_env);
  if (value == NULL) {
    malatom_free(ast);
    return NULL;
  }

  char *key_str = strdup(key->value.symbol);
  malatom_free(ast);

  MalAtom *value_copy = malatom_copy(value);
  if (value_copy == NULL) {
    free(key_str);
    malatom_free(value);
    return NULL;
  }
  if (env_set(repl_env, key_str, value, (void (*)(void *))malatom_free)) {
    return NULL;
  }

  return value_copy;
}

MalAtom *eval_let(MalAtom *ast, Env *repl_env) {
  if (ast == NULL) {
    return NULL;
  }

  if (ast->type != MAL_ATOM_LIST) {
    fprintf(stderr, "Expected a list\n");
    malatom_free(ast);
    return NULL;
  }

  MalAtom *symbol = ast->value.children;
  if (symbol == NULL || symbol->type != MAL_SYMBOL ||
      strcmp(symbol->value.symbol, "let*") != 0) {
    fprintf(stderr, "Expected a symbol 'let*'\n");
    malatom_free(ast);
    return NULL;
  }

  MalAtom *bindings = symbol->next;
  if (bindings == NULL ||
      (bindings->type != MAL_ATOM_LIST && bindings->type != MAL_VECTOR)) {
    fprintf(stderr, "Expected a list or vector\n");
    malatom_free(ast);
    return NULL;
  }

  Env *let_env = env_new(repl_env, DEFAULT_CONTAINER_CAPACITY);
  if (let_env == NULL) {
    perror("Failed to allocate memory for environment");
    malatom_free(ast);
    return NULL;
  }

  if (bindings->type == MAL_ATOM_LIST) {
    // Delete the current element from the list
    MalAtom *head = bindings->value.children;
    bindings->value.children = NULL;
    for (MalAtom *key = head, *next = NULL; key != NULL; key = next) {
      if (key->type != MAL_SYMBOL) {
        fprintf(stderr, "Expected a symbol\n");
        malatom_free(ast);
        env_free(let_env);
        return NULL;
      }

      MalAtom *value = key->next;
      if (value == NULL) {
        fprintf(stderr, "Expected a value\n");
        malatom_free(ast);
        env_free(let_env);
        return NULL;
      }
      next = value->next;

      // Delete the current element from the list
      key->next = next;
      value->next = NULL;
      value = EVAL(value, let_env);
      if (value == NULL) {
        malatom_free(ast);
        env_free(let_env);
        return NULL;
      }

      char *key_str = strdup(key->value.symbol);
      key->next = NULL;
      malatom_free(key);

      if (env_set(let_env, key_str, value, (void (*)(void *))malatom_free)) {
        malatom_free(ast);
        env_free(let_env);
        return NULL;
      }
    }
  } else {
    for (int i = 0; i < bindings->value.vector->size; i++) {
      MalAtom *key = bindings->value.vector->buffer[i];
      bindings->value.vector->buffer[i] = NULL;
      if (key->type != MAL_SYMBOL) {
        fprintf(stderr, "Expected a symbol\n");
        malatom_free(ast);
        env_free(let_env);
        return NULL;
      }

      MalAtom *value = bindings->value.vector->buffer[++i];
      bindings->value.vector->buffer[i] = NULL;
      if (i >= bindings->value.vector->size) {
        fprintf(stderr, "Expected a value\n");
        malatom_free(ast);
        env_free(let_env);
        return NULL;
      }

      value = EVAL(value, let_env);
      if (value == NULL) {
        malatom_free(ast);
        env_free(let_env);
        return NULL;
      }

      char *key_str = strdup(key->value.symbol);
      malatom_free(key);

      if (env_set(let_env, key_str, value, (void (*)(void *))malatom_free)) {
        malatom_free(ast);
        env_free(let_env);
        return NULL;
      }
    }
  }

  MalAtom *body = bindings->next;
  if (body == NULL || body->next != NULL) {
    fprintf(stderr, "Expected a body\n");
    malatom_free(ast);
    return NULL;
  }

  // Delete the current element from the list
  bindings->next = NULL;

  MalAtom *result = EVAL(body, let_env);
  malatom_free(ast);
  env_free(let_env);

  return result;
}

MalAtom *EVAL(MalAtom *atom, Env *repl_env) {
  if (atom == NULL) {
    return NULL;
  }

  if (atom->type == MAL_ATOM_LIST) {
    if (atom->value.children == NULL) {
      return atom;
    }
    MalAtom *first = atom->value.children;
    if (first->type == MAL_SYMBOL) {
      if (strcmp(first->value.symbol, "def!") == 0) {
        return eval_def(atom, repl_env);
      }

      if (strcmp(first->value.symbol, "let*") == 0) {
        return eval_let(atom, repl_env);
      }
    }

    MalAtom *list = eval_ast(atom, repl_env);
    atom = NULL;
    if (list == NULL) {
      return NULL;
    }
    if (list->value.children == NULL) {
      return list;
    }
    first = list->value.children;
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
char *rep(const char *str, Env *repl_env) {
  char *result = PRINT(EVAL(READ(str), repl_env));
  return result;
}

Env *init_env() {
  Env *repl_env = env_new(NULL, DEFAULT_CONTAINER_CAPACITY);
  if (repl_env == NULL) {
    perror("Failed to allocate memory for environment");
    return NULL;
  }

  MalAtom *plus_atom = malatom_new(MAL_FUNCTION);
  if (plus_atom == NULL) {
    perror("Failed to allocate memory for atom");
    env_free(repl_env);
    return NULL;
  }
  plus_atom->value.function = (void *(*)(void *))plus;
  char *plus_str = strdup("+");
  if (env_set(repl_env, plus_str, plus_atom, (void (*)(void *))malatom_free)) {
    free(plus_str);
    malatom_free(plus_atom);
    env_free(repl_env);
    return NULL;
  }
  plus_str = NULL;
  plus_atom = NULL;

  MalAtom *minus_atom = malatom_new(MAL_FUNCTION);
  if (minus_atom == NULL) {
    perror("Failed to allocate memory for atom");
    env_free(repl_env);
    return NULL;
  }
  minus_atom->value.function = (void *(*)(void *))minus;
  char *minus_str = strdup("-");
  if (env_set(repl_env, minus_str, minus_atom,
              (void (*)(void *))malatom_free)) {
    free(minus_str);
    malatom_free(minus_atom);
    env_free(repl_env);
    return NULL;
  }
  minus_str = NULL;
  minus_atom = NULL;

  MalAtom *multiply_atom = malatom_new(MAL_FUNCTION);
  if (multiply_atom == NULL) {
    perror("Failed to allocate memory for atom");
    env_free(repl_env);
    return NULL;
  }
  multiply_atom->value.function = (void *(*)(void *))multiply;
  char *multiply_str = strdup("*");
  if (env_set(repl_env, multiply_str, multiply_atom,
              (void (*)(void *))malatom_free)) {
    free(multiply_str);
  }
  multiply_str = NULL;
  multiply_atom = NULL;

  MalAtom *divide_atom = malatom_new(MAL_FUNCTION);
  if (divide_atom == NULL) {
    perror("Failed to allocate memory for atom");
    env_free(repl_env);
    return NULL;
  }
  divide_atom->value.function = (void *(*)(void *))divide;
  char *divide_str = strdup("/");
  if (env_set(repl_env, divide_str, divide_atom,
              (void (*)(void *))malatom_free)) {
    free(divide_str);
    malatom_free(divide_atom);
    env_free(repl_env);
    return NULL;
  }
  divide_str = NULL;
  divide_atom = NULL;

  return repl_env;
}

int main(int argc, char **argv) {
  if (regex_compile()) {
    return 1;
  }
  Env *repl_env = init_env();
  if (repl_env == NULL) {
    regex_free();
    return 1;
  }

  while (1) {
    const char *input = readline(PROMPT);
    if (input == NULL) {
      break;
    }
    add_history(input);
    const char *output = rep(input, repl_env);
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

  env_free(repl_env);
  regex_free();

  return 0;
}
