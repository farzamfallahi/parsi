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
        unsigned long c;

        if (len < 2) return 0;                    /* character is cut off */
        if ((s[1] & 0xC0) != 0x80) return 0;      /* 2nd byte must be 10xxxxxx */

        c = ((unsigned long)(s[0] & 0x1F) << 6)   /* 5 bits from byte 1 */
          |  (unsigned long)(s[1] & 0x3F);        /* 6 bits from byte 2 */

        if (c < 0x80) return 0;                   /* overlong encoding */

        *cp = c;
        return 2;
    }
    
    /* 3 bytes: 1110xxxx 10xxxxxx 10xxxxxx (ZWNJ lives here) */
    if ((s[0] & 0xF0) == 0xE0) {
        unsigned long c;

        if (len < 3) return 0;                    /* character is cut off */
        if ((s[1] & 0xC0) != 0x80) return 0;      /* 2nd byte must be 10xxxxxx */
        if ((s[2] & 0xC0) != 0x80) return 0;      /* 3rd byte must be 10xxxxxx */

        c = ((unsigned long)(s[0] & 0x0F) << 12)  /* 4 bits from byte 1 */
          | ((unsigned long)(s[1] & 0x3F) << 6)   /* 6 bits from byte 2 */
          |  (unsigned long)(s[2] & 0x3F);        /* 6 bits from byte 3 */

        if (c < 0x800) return 0;                  /* overlong encoding */
        if (c >= 0xD800 && c <= 0xDFFF) return 0; /* surrogates are invalid */

        *cp = c;
        return 3;
    }

    /* 4 bytes: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (emoji live here) */
    if ((s[0] & 0xF8) == 0xF0) {
        unsigned long c;

        if (len < 4) return 0;                    /* character is cut off */
        if ((s[1] & 0xC0) != 0x80) return 0;      /* 2nd byte must be 10xxxxxx */
        if ((s[2] & 0xC0) != 0x80) return 0;      /* 3rd byte must be 10xxxxxx */
        if ((s[3] & 0xC0) != 0x80) return 0;      /* 4th byte must be 10xxxxxx */

        c = ((unsigned long)(s[0] & 0x07) << 18)  /* 3 bits from byte 1 */
          | ((unsigned long)(s[1] & 0x3F) << 12)  /* 6 bits from byte 2 */
          | ((unsigned long)(s[2] & 0x3F) << 6)   /* 6 bits from byte 3 */
          |  (unsigned long)(s[3] & 0x3F);        /* 6 bits from byte 4 */

        if (c < 0x10000) return 0;                /* overlong encoding */
        if (c > 0x10FFFF) return 0;               /* beyond Unicode range */

        *cp = c;
        return 4;
    }

    return 0; /* invalid lead byte (continuation byte, or 5+ byte form) */
}

#endif /* PARSI_IMPLEMENTATION */