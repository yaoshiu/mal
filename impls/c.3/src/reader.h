#ifndef MAL_READER_H
#define MAL_READER_H

#include "types.h"

typedef struct Token {
  struct Token *next, *prev;
  char *str;
} Token;

// Create a new `Token` object.
//
// The token should be freed with `token_free`.
Token *token_new(const char *str);

// Free a `Token` object.
//
// Take the `token` ownership.
void token_free(Token *token);

typedef struct Tokens {
  Token *head, *tail;
} Tokens;

// Compile the regex.
//
// Return 0 on success, -1 on failure.
// The regex should be freed with `regex_free`.
int regex_compile();

// Free the regex.
void regex_free();

// Create a new `Tokens` object.
//
// The `tokens` should be freed with `tokens_free()`.
Tokens *tokens_new();

// Free a `Tokens` object.
//
// Took the `tokens` ownership.
void tokens_free(Tokens *tokens);

// Add a new `token` (copy) to the end of `tokens`.
int tokens_push(Tokens *tokens, const char *token);

// Remove the last token from `tokens`.
int tokens_pop(Tokens *tokens);

typedef struct Reader {
  Tokens *tokens;
  Token *current;
} Reader;

// Create a new `Reader` object.
//
// Reader should be freed with `reader_free` by the caller.
// Take the `tokens` ownership. Which means that the tokens should only be freed
// with `reader_free` after passed to this function.
Reader *reader_new(Tokens *tokens);

// Free a Reader object.
void reader_free(Reader *reader);

// Return the next token from `reader`.
//
// The returned token should not be freed since it is owned by reader,
// which means the caller should in most cases copy the token.
const char *reader_next(Reader *reader);

// Return the next token from `reader` without moving the position.
//
// The returned token should not be freed since it is owned by reader;
// which means the caller should in most cases copy the token.
const char *reader_peek(const Reader *reader);

// Return a `MalAtom` from a string.
MalAtom *read_str(const char *str);

// Return a `Tokens` object from a string.
Tokens *tokenize(const char *str);

// The returned `MalAtom` should be freed with `malatom_free` by the caller.
MalAtom *read_from(Reader *reader);

// The returned `MalAtom` should be freed with `malatom_free` by the caller.
MalAtom *read_list(Reader *reader);

// The returned `MalAtom` should be freed with `malatom_free`.
MalAtom *read_atom(Reader *reader);

// The ruturned string should be freed with `free`.
char *read_atom_string(const char *token);

// The returned `MalAtom` should be freed with `malatom_free` by the caller.
MalVector *read_atom_vector(Reader *reader);

// The returned `MalAtom` should be freed with `malatom_free` by the caller.
MalHashmap *read_atom_hashmap(Reader *reader);

// The returned `MalAtom` should be freed with `malatom_free` by the caller.
MalAtom *read_metadata(Reader *reader);

// The returned `MalAtom` should be freed with `malatom_free` by the caller.
MalAtom *read_quotes(Reader *reader, const char *token);

#endif /* MAL_READER_H */
