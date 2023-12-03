#ifndef MAL_READER_H
#define MAL_READER_H

#include "types.h"

typedef struct Token {
  struct Token *next, *prev;
  char *str;
} Token;

/**
 * Allocates and initializes a new Token.
 *
 * Parameters:
 * - str: String value for token. Caller retains ownership.
 *
 * Returns:
 * - New Token struct. Caller takes ownership.
 *   Must be freed later with token_free().
 */
Token *token_new(const char *str);

/**
 * Frees memory for a Token.
 *
 * Parameters:
 * - token: Token to free. Takes ownership from caller.
 */
void token_free(Token *token);

typedef struct Tokens {
  Token *head, *tail;
} Tokens;

/**
 * Compiles the regular expression used for tokenization.
 *
 * Returns:
 * - 0 on success, 1 on failure to compile regex.
 *
 * The compiled regex is stored globally in the regex variable.
 * Must be freed later with regex_free().
 */
int regex_compile();

/**
 * Frees compiled regular expression.
 */
void regex_free();

/**
 * Allocates and initializes a new Tokens container.
 *
 * Returns:
 * - New Tokens container. Caller takes ownership.
 *   Must be freed later with tokens_free().
 */
Tokens *tokens_new();

/**
 * Frees Tokens container and all tokens.
 *
 * Parameters:
 * - tokens: Tokens to free. Takes ownership from caller.
 */
void tokens_free(Tokens *tokens);

/**
 * Pushes a token to the end of a Tokens list.
 *
 * Parameters:
 * - tokens: Tokens container to push to. Caller retains ownership.
 * - token: Token string to add. Caller retains ownership.
 *
 * Returns:
 * - 0 on success, 1 on failure.
 */
int tokens_push(Tokens *tokens, const char *token);

// Remove the last token from `tokens`.
int tokens_pop(Tokens *tokens);

typedef struct Reader {
  Tokens *tokens;
  Token *current;
} Reader;

/**
 * Allocates and initializes new Reader.
 *
 * Parameters:
 * - tokens: Token container for reader. Caller retains ownership.
 *
 * Returns:
 * - New Reader instance. Caller takes ownership,
 *   Must be freed later with reader_free().
 */
Reader *reader_new(Tokens *tokens);

/**
 * Frees a Reader instance.
 *
 * Parameters:
 * - reader: Reader to free. Takes ownership from caller.
 */
void reader_free(Reader *reader);

/**
 * Gets next token from reader.
 *
 * Parameters:
 * - reader: Reader to advance. Caller retains ownership.
 *
 * Returns:
 * - Next token string. Caller does NOT take ownership.
 */
const char *reader_next(Reader *reader);

/**
 * Gets current token without advancing.
 *
 * Parameters:
 * - reader: Reader to peek from. Caller retains ownership.
 *
 * Returns:
 * - Current token string. Caller does NOT take ownership.
 */
const char *reader_peek(const Reader *reader);

/**
 * Parses a string into Mal atoms.
 *
 * Parameters:
 * - str: String to parse. Caller retains ownership.
 *
 * Returns:
 * - MalAtom from parsing string. Caller takes ownership.
 *   Must be freed later with malatom_free().
 */
MalAtom *read_str(const char *str);

/**
 * Tokenizes a string into tokens.
 *
 * Parameters:
 * - str: Input string to tokenize. Caller retains ownership.
 *
 * Returns:
 * - Tokens container with tokenized strings. Caller takes ownership.
 *   Must be freed later with tokens_free().
 */
Tokens *tokenize(const char *str);

/**
 * Reads a Mal list from the reader.
 *
 * Parameters:
 * - reader: Reader to read from. Caller retains ownership.
 *
 * Returns:
 * - MalAtom of type MAL_LIST. Caller takes ownership.
 *   Must be freed later with malatom_free().
 */
MalAtom *read_from(Reader *reader);

/**
 * Reads a Mal vector from the reader.
 *
 * Parameters:
 * - reader: Reader to read from. Caller retains ownership.
 *
 * Returns:
 * - MalVector struct. Caller takes ownership.
 *   Must be freed later with malvector_free().
 */
MalAtom *read_list(Reader *reader);

/**
 * Reads a single Mal atom from the reader.
 *
 * Parameters:
 * - reader: Reader to parse atom from. Caller retains ownership.
 *
 * Returns:
 * - Next MalAtom from reader. Caller takes ownership.
 */
MalAtom *read_atom(Reader *reader);

/**
 * Reads a string atom from reader.
 *
 * Parameters:
 * - token: Current token containing quote. Caller retains ownership.
 *
 * Returns:
 * - New MalAtom string. Caller takes ownership.
 */
char *read_atom_string(const char *token);

/**
 * Parses vector from reader.
 *
 * Parameters:
 * - reader: Reader to parse from. Caller retains ownership.
 *
 * Returns:
 * - MalVector parsed from reader. Caller takes ownership.
 */
MalVector *read_atom_vector(Reader *reader);

/**
 * Parses a hashmap from the reader.
 *
 * Parameters:
 * - reader: Reader to parse from. Caller retains ownership.
 *
 * Returns:
 * - MalHashmap parsed from reader. Caller takes ownership.
 */
MalHashmap *read_atom_hashmap(Reader *reader);

/**
 * Reads and parses a metadata annotation.
 *
 * Parameters:
 * - reader: Reader to parse metadata from.
 *            Caller retains ownership.
 *
 * Returns:
 * - MalAtom with metadata. Caller takes ownership.
 */
MalAtom *read_metadata(Reader *reader);

/**
 * Reads and parses quoted Mal atoms.
 *
 * Parameters:
 * - reader: Reader to parse from. Caller retains ownership.
 * - token: Current token with quote. Caller retains ownership.
 *
 * Returns:
 * - MalAtom with quoted form. Caller takes ownership.
 */
MalAtom *read_quotes(Reader *reader, const char *token);

#endif /* MAL_READER_H */
