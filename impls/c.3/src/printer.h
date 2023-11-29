#ifndef MAL_PRINTER_H
#define MAL_PRINTER_H

#include <stdbool.h>

#include "types.h"

// Returns a string representation of atom.
//
// The returned string should be freed with `free` by the caller.
char *pr_str(const MalAtom *atom, const bool print_readably);

#endif
