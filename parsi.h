/* parsi.h - Persian text utilities in C (single-header library)
 *
 * In ONE .c file:
 *     #define PARSI_IMPLEMENTATION
 *     #include "parsi.h"
 * Everywhere else, just #include "parsi.h".
 */
#ifndef PARSI_H
#define PARSI_H

#include <stddef.h>

/* Public API goes here */

/* Decode one UTF-8 character from s (at most len bytes).
 * On success, stores the code point in *cp and returns the number
 * of bytes used (1-4). Returns 0 if the input is not valid UTF-8. */
size_t parsi_utf8_decode(const unsigned char *s, size_t len, unsigned long *cp);

#endif /* PARSI_H */

#ifdef PARSI_IMPLEMENTATION

/* Implementation goes here */

size_t parsi_utf8_decode(const unsigned char *s, size_t len, unsigned long *cp)
{
    if (len == 0) return 0;

    /* 1 byte: 0xxxxxxx (ASCII) */
    if (s[0] < 0x80) {
        *cp = s[0];
        return 1;
    }

    /* 2 bytes: 110xxxxx 10xxxxxx (Persian letters live here) */
    if ((s[0] & 0xE0) == 0xC0) {
        /* TODO: your code here */
    }

    return 0; /* invalid, or a length not supported yet */
}

#endif /* PARSI_IMPLEMENTATION */