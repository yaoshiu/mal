#include <ctype.h>
#include <pcre.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "consts.h"
#include "printer.h"
#include "reader.h"

Token *token_new(const char *str) {
  Token *token = (Token *)malloc(sizeof(Token));
  if (token == NULL) {
    perror("Failed to allocate memory for token");
    return NULL;
  }
  token->str = strdup(str);
  if (token->str == NULL) {
    perror("Failed to allocate memory for token string");
    token_free(token);
    return NULL;
  }
  token->next = NULL;
  token->prev = NULL;
  return token;
}

void token_free(Token *token) {
  if (token != NULL) {
    if (token->str != NULL) {
      free(token->str);
    }
    free(token);
  }
}

Tokens *tokens_new() {
  Tokens *tokens = (Tokens *)malloc(sizeof(Tokens));
  if (tokens == NULL) {
    perror("Failed to allocate memory for tokens");
    return NULL;
  }
  tokens->head = NULL;
  tokens->tail = NULL;
  return tokens;
}

void tokens_free(Tokens *tokens) {
  Token *token = tokens->head;
  while (token != NULL) {
    Token *next = token->next;
    token_free(token);
    token = next;
  }
  free(tokens);
}

int tokens_push(Tokens *tokens, const char *token) {
  Token *new_token = token_new(token);
  if (new_token == NULL) {
    return 1;
  }

  if (tokens->head == NULL) {
    tokens->head = new_token;
    tokens->tail = new_token;
  } else {
    tokens->tail->next = new_token;
    new_token->prev = tokens->tail;
    tokens->tail = new_token;
  }

  return 0;
}

Reader *reader_new(Tokens *tokens) {
  Reader *reader = (Reader *)malloc(sizeof(Reader));
  if (reader == NULL) {
    perror("Failed to allocate memory for reader");
    return NULL;
  }
  reader->tokens = tokens;
  reader->current = tokens->head;
  return reader;
}

void reader_free(Reader *reader) {
  if (reader != NULL) {
    if (reader->tokens != NULL) {
      tokens_free(reader->tokens);
    }
    free(reader);
  }
}

const char *reader_next(Reader *reader) {
  if (reader->current == NULL) {
    // EOF
    return NULL;
  }

  const char *token = reader->current->str;
  reader->current = reader->current->next;
  return token;
}

const char *reader_peek(const Reader *reader) {
  if (reader->current == NULL) {
    // EOF
    return NULL;
  }

  return reader->current->str;
}

MalAtom *read_str(const char *str) {
  Tokens *tokens = tokenize(str);
  if (tokens == NULL) {
    return NULL;
  }
  Reader *reader = reader_new(tokens);
  MalAtom *atom = read_from(reader);
  reader_free(reader);

  return atom;
}

static pcre *regex = NULL;

int regex_compile() {
  const char *error;
  int error_offset;

  regex = pcre_compile(
      "[\\s ,]*(~@|[\\[\\]{}()'`~@\\^]|\"(?:[\\\\].|[^\\\\\"])*\"?|;.*|[^\\s "
      "\\[\\]{}()'\"`~@,;]*)",
      0, &error, &error_offset, NULL);

  if (regex == NULL) {
    fprintf(stderr, "PCRE compilation failed at offset %d: %s\n", error_offset,
            error);
    return 1;
  }

  return 0;
}

void regex_free() { pcre_free(regex); }

Tokens *tokenize(const char *str) {
  Tokens *tokens = tokens_new();
  if (tokens == NULL) {
    return NULL;
  }

  int offset = 0;
  int len = strlen(str);
  while (offset < len) {
    int ovector[BUFFER_SIZE];
    int rc = pcre_exec(regex, NULL, str, len, offset, 0, ovector, BUFFER_SIZE);
    if (rc < 0) {
      if (rc == PCRE_ERROR_NOMATCH) {
        fprintf(stderr, "No match found\n");
      } else {
        fprintf(stderr, "Matching error %d\n", rc);
      }
      tokens_free(tokens);
      return NULL;
    }

    for (int i = 1; i < rc; i += 2) {
      int start = ovector[2 * i];
      int end = ovector[2 * i + 1];
      int token_len = end - start;
      char token[token_len + 1];
      strncpy(token, str + start, token_len);
      token[token_len] = '\0';
      if (tokens_push(tokens, token)) {
        tokens_free(tokens);
        return NULL;
      }
    }

    offset = ovector[1];
  }

  return tokens;
}

MalAtom *read_from(Reader *reader) {
  const char *peek = reader_peek(reader);
  if (peek != NULL && peek[0] == '(') {
    reader_next(reader);
    return read_list(reader);
  } else {
    return read_atom(reader);
  }
}

MalAtom *read_list(Reader *reader) {
  MalAtom *list = malatom_new(MAL_ATOM_LIST);
  MalAtom *tail = NULL;

  while (true) {
    MalAtom *atom = read_from(reader);
    if (atom == NULL) {
      malatom_free(list);
      return NULL;
    }
    if (atom->type == MAL_EOF) {
      fprintf(stderr, "Unexpected EOF while reading list\n");
      malatom_free(atom);
      return NULL;
    }

    if (atom->type == MAL_SYMBOL && atom->value.symbol != NULL &&
        atom->value.symbol[0] == ')') {
      return list;
    }

    // Take ownership of the atom.
    if (list->value.children == NULL) {
      list->value.children = atom;
      tail = atom;
    } else {
      tail->next = atom;
      tail = atom;
    }
  }
}

MalAtom *read_atom(Reader *reader) {
  MalAtom *atom;
  const char *token = reader_next(reader);
  if (token == NULL) {
    atom = malatom_new(MAL_EOF);
    if (atom == NULL) {
      return NULL;
    }
    return atom;
  }

  bool is_int = true;
  for (int i = 0; token[i] != '\0'; i++) {
    if (!isdigit(token[i])) {
      is_int = false;
      break;
    }
  }

  if (is_int) {
    atom = malatom_new(MAL_INT);
    if (atom == NULL) {
      return NULL;
    }
    atom->value.digit = atoi(token);

  } else if (token[0] == '[') {
    atom = malatom_new(MAL_VECTOR);
    if (atom == NULL) {
      return NULL;
    }
    MalVector *vector = read_atom_vector(reader);
    if (vector == NULL) {
      malatom_free(atom);
      return NULL;
    }
    atom->value.vector = vector;

  } else if (token[0] == '^') {
    atom = read_metadata(reader);
    if (atom == NULL) {
      return NULL;
    }

  } else if (strchr("'`~@", token[0]) != NULL) {
    atom = read_quotes(reader, token);
    if (atom == NULL) {
      return NULL;
    }

  } else if (token[0] == ':') {
    atom = malatom_new(MAL_KEYWORD);
    if (atom == NULL) {
      return NULL;
    }
    atom->value.keyword = strdup(token + 1);

  } else if (token[0] == '{') {
    atom = malatom_new(MAL_HASHMAP);
    MalHashmap *map = read_atom_hashmap(reader);
    if (map == NULL) {
      malatom_free(atom);
      return NULL;
    }
    atom->value.hashmap = map;

  } else if (token[0] == '"') {
    char *str = read_atom_string(token);
    if (str == NULL) {
      return NULL;
    }
    atom = malatom_new(MAL_STRING);
    if (atom == NULL) {
      free(str);
      return NULL;
    }
    atom->value.string = str;

  } else if (!strcmp(token, "true") || !strcmp(token, "false")) {
    atom = malatom_new(MAL_BOOL);
    if (atom == NULL) {
      return NULL;
    }
    atom->value.boolean = !strcmp(token, "true");

  } else if (!strcmp(token, "nil")) {
    atom = malatom_new(MAL_NIL);
    if (atom == NULL) {
      return NULL;
    }
    atom->value.symbol = NULL;

  } else {
    atom = malatom_new(MAL_SYMBOL);
    if (atom == NULL) {
      return NULL;
    }
    atom->value.symbol = strdup(token);
  }

  return atom;
}

char *read_atom_string(const char *token) {
  int len = strlen(token);
  if (len < 2 || token[len - 1] != '"') {
    fprintf(stderr, "Unexpected EOF while reading string\n");
    return NULL;
  }
  char *str = (char *)calloc(len - 1, sizeof(char));
  if (str == NULL) {
    perror("Failed to allocate memory for string");
    return NULL;
  }
  int j = 0;
  for (int i = 1; i < len - 1; i++) {
    if (token[i] == '\\') {
      if (i == len - 2) {
        fprintf(stderr, "Unexpected EOF while reading string\n");
        free(str);
        return NULL;
      }
      switch (token[++i]) {
      case 'n':
        str[j++] = '\n';
        break;
      case 't':
        str[j++] = '\t';
        break;
      case 'r':
        str[j++] = '\r';
        break;
      case 'b':
        str[j++] = '\b';
        break;
      case 'f':
        str[j++] = '\f';
        break;
      case '\\':
        str[j++] = '\\';
        break;
      case '"':
        str[j++] = '"';
        break;
      default:
        fprintf(stderr, "Unknown escape character: %c\n", token[i]);
        free(str);
        return NULL;
      }
    } else {
      str[j++] = token[i];
    }
  }
  str[j] = '\0';

  return str;
}

MalVector *read_atom_vector(Reader *reader) {
  MalVector *vector = malvector_new(DEFAULT_CONTAINER_CAPACITY);
  if (vector == NULL) {
    return NULL;
  }

  while (true) {
    const char *peek = reader_peek(reader);
    if (peek == NULL) {
      fprintf(stderr, "Unexpected EOF while reading vector\n");
      malvector_free(vector);
      return NULL;
    }
    if (peek[0] == ']') {
      reader_next(reader);
      return vector;
    }

    MalAtom *atom = read_from(reader);
    if (atom == NULL) {
      malvector_free(vector);
      return NULL;
    }
    if (atom->type == MAL_EOF) {
      fprintf(stderr, "Unexpected EOF while reading vector\n");
      malvector_free(vector);
      malatom_free(atom);
      return NULL;
    }
    if (malvector_push(vector, atom)) {
      malvector_free(vector);
      malatom_free(atom);
      return NULL;
    }
  }
}

MalHashmap *read_atom_hashmap(Reader *reader) {
  MalHashmap *map = malhashmap_new(DEFAULT_CONTAINER_CAPACITY);
  if (map == NULL) {
    return NULL;
  }

  while (true) {
    const char *peek = reader_peek(reader);
    if (peek == NULL) {
      fprintf(stderr, "Unexpected EOF while reading hashmap\n");
      malhashmap_free(map);
      return NULL;
    }
    if (peek[0] == '}') {
      reader_next(reader);
      return map;
    }

    MalAtom *key = read_from(reader);
    if (key == NULL) {
      malhashmap_free(map);
      return NULL;
    }
    if (key->type == MAL_EOF) {
      fprintf(stderr, "Unexpected EOF while reading hashmap\n");
      malhashmap_free(map);
      malatom_free(key);
      return NULL;
    }

    MalAtom *value = read_from(reader);
    if (value == NULL) {
      malhashmap_free(map);
      malatom_free(key);
      return NULL;
    }
    if (value->type == MAL_EOF) {
      fprintf(stderr, "Unexpected EOF while reading hashmap\n");
      malhashmap_free(map);
      malatom_free(key);
      malatom_free(value);
      return NULL;
    }

    if (malhashmap_insert(map, key, value)) {
      malhashmap_free(map);
      malatom_free(key);
      malatom_free(value);
      return NULL;
    }
  }
}

MalAtom *read_metadata(Reader *reader) {
  MalAtom *atom = malatom_new(MAL_ATOM_LIST);
  if (atom == NULL) {
    return NULL;
  }

  atom->value.children = malatom_new(MAL_SYMBOL);
  if (atom->value.children == NULL) {
    malatom_free(atom);
    return NULL;
  }
  atom->value.children->value.symbol = strdup("with-meta");

  MalAtom *next = read_from(reader);
  if (next == NULL) {
    malatom_free(atom);
    return NULL;
  }
  if (next->type == MAL_EOF) {
    fprintf(stderr, "Unexpected EOF while reading atom\n");
    malatom_free(atom);
    malatom_free(next);
    return NULL;
  }
  MalAtom *nnext = read_from(reader);
  if (nnext == NULL) {
    malatom_free(atom);
    malatom_free(next);
    return NULL;
  }
  if (nnext->type == MAL_EOF) {
    fprintf(stderr, "Unexpected EOF while reading atom\n");
    malatom_free(atom);
    malatom_free(next);
    malatom_free(nnext);
    return NULL;
  }
  atom->value.children->next = nnext;
  nnext = NULL;

  MalAtom *key, *value;
  if (next->type == MAL_HASHMAP) {
    atom->value.children->next->next = next;
    next = NULL;
  } else {
    atom->value.children->next->next = malatom_new(MAL_HASHMAP);
    if (atom->value.children->next->next == NULL) {
      perror("Failed to allocate memory for atom");
      malatom_free(next);
      malatom_free(atom);
      return NULL;
    }
    atom->value.children->next->next->value.hashmap = malhashmap_new(1);
    if (atom->value.children->next->next->value.hashmap == NULL) {
      malatom_free(next);
      malatom_free(atom);
      return NULL;
    }

    switch (next->type) {
    case MAL_SYMBOL:
    case MAL_STRING:
      key = malatom_new(MAL_KEYWORD);
      key->value.keyword = strdup("tag");
      value = next;
      next = NULL;
      break;
    case MAL_KEYWORD:
      value = malatom_new(MAL_BOOL);
      value->value.boolean = true;
      key = next;
      next = NULL;
      break;
    default:
      fprintf(stderr, "Invalid metadata type\n");
      malatom_free(atom);
      malatom_free(next);
      return NULL;
    }

    if (malhashmap_insert(atom->value.children->next->next->value.hashmap, key,
                          value)) {
      malatom_free(atom);
      return NULL;
    }
  }

  return atom;
}

MalAtom *read_quotes(Reader *reader, const char *token) {
  MalAtom *atom = malatom_new(MAL_ATOM_LIST);
  if (atom == NULL) {
    return NULL;
  }
  atom->value.children = malatom_new(MAL_SYMBOL);
  switch (token[0]) {
  case '\'':
    atom->value.children->value.symbol = strdup("quote");
    break;
  case '`':
    atom->value.children->value.symbol = strdup("quasiquote");
    break;
  case '~':
    if (strlen(token) > 1 && token[1] == '@') {
      atom->value.children->value.symbol = strdup("splice-unquote");
    } else {
      atom->value.children->value.symbol = strdup("unquote");
    }
    break;
  case '@':
    atom->value.children->value.symbol = strdup("deref");
    break;
  default:
    fprintf(stderr, "Invalid quote type\n");
    malatom_free(atom);
    return NULL;
  }
  MalAtom *next = read_from(reader);
  atom->value.children->next = next;
  if (next == NULL) {
    malatom_free(atom);
    return NULL;
  }
  if (next->type == MAL_EOF) {
    fprintf(stderr, "Unexpected EOF while reading atom\n");
    malatom_free(atom);
    return NULL;
  }

  return atom;
}
