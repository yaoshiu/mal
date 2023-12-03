#ifndef MAL_PRINTER_H
#define MAL_PRINTER_H

#include <stdbool.h>

#include "types.h"

/**
 * Prints a MalAtom struct to a string.
 *
 * Parameters:
 * - atom: MalAtom to print. Caller retains ownership.
 * - print_readably: Whether to print string escapes for a reader.
 *
 * Return:
 * - String representation of atom. Caller takes ownership.
 *   Needs to be freed later.
 */
char *pr_str(const MalAtom *atom, const bool print_readably);

#endif
