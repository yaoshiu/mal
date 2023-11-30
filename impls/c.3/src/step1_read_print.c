#include <stdio.h>
#include <stdlib.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "printer.h"
#include "reader.h"

#define PROMPT "user> "

// Read a str and return a MalAtom
//
// The returned MalAtom should be freed by the caller.
MalAtom *READ(const char *str) { return read_str(str); }

MalAtom *EVAL(MalAtom *atom) { return atom; }

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

// Read a str, evaluate it, print the result and return a string
//
// The returned string should be freed by the caller.
char *rep(const char *str) { return PRINT(EVAL(READ(str))); }

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
