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

// Frees the atom.
char *PRINT(MalAtom *atom) {
  char *str = pr_str(atom, true);
  if (atom == NULL) {
    return NULL;
  }
  malatom_free(atom);
  return str;
}

char *rep(char *str) { return PRINT(EVAL(READ(str))); }

int main(int argc, char **argv) {
  char *input;
  if (regex_compile()) {
    return 1;
  }

  while (1) {
    input = readline(PROMPT);
    add_history(input);
    char *output = rep(input);
    if (output == NULL) {
      continue;
    }
    printf("%s\n", output);
  }
  free(input);
  regex_free();

  return 0;
}
