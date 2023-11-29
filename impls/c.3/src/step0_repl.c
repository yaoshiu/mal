#include <stdio.h>

#include <readline/history.h>
#include <readline/readline.h>

#define PROMPT "user> "

char *READ(char *str) { return str; }

char *EVAL(char *str) { return str; }

char *PRINT(char *str) { return str; }

char *rep(char *str) { return PRINT(EVAL(READ(str))); }

int main(int argc, char **argv) {
  char *input;

  while (1) {
    input = readline(PROMPT);
    add_history(input);
    printf("%s\n", rep(input));
  }

  return 0;
}
