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

/* Encode code point cp as UTF-8 into out (must have room for 4 bytes).
 * Returns the number of bytes written (1-4), or 0 if cp is not a valid
 * Unicode scalar value (a surrogate or above U+10FFFF). */
size_t parsi_utf8_encode(unsigned long cp, unsigned char *out);

/* Flags for parsi_normalize */
#define PARSI_YEH_KAF 0x01u /* Arabic yeh/alef maksura -> Persian yeh, Arabic kaf -> Persian kaf */

/* Normalize in_len bytes of UTF-8 text from in into out, according to flags.
 * Like snprintf: writes at most out_cap bytes including a terminating NUL
 * (out may be NULL when out_cap is 0), never splits a character, and returns
 * the length the full result needs, not counting the NUL. If the return value
 * is >= out_cap, the output was truncated. Invalid UTF-8 bytes are copied
 * through unchanged. */
size_t parsi_normalize(const char *in, size_t in_len, char *out, size_t out_cap,
                       unsigned flags);

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

size_t parsi_utf8_encode(unsigned long cp, unsigned char *out)
{
    /* 1 byte: 0xxxxxxx (ASCII) */
    if (cp < 0x80) {
        out[0] = (unsigned char)cp;
        return 1;
    }

    /* 2 bytes: 110xxxxx 10xxxxxx (Persian letters live here) */
    if (cp < 0x800) {
        out[0] = (unsigned char)(0xC0 | (cp >> 6));            /* top 5 bits */
        out[1] = (unsigned char)(0x80 | (cp & 0x3F));          /* low 6 bits */
        return 2;
    }

    /* 3 bytes: 1110xxxx 10xxxxxx 10xxxxxx (ZWNJ lives here) */
    if (cp < 0x10000) {
        if (cp >= 0xD800 && cp <= 0xDFFF) return 0;            /* surrogates are invalid */

        out[0] = (unsigned char)(0xE0 | (cp >> 12));           /* top 4 bits */
        out[1] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));   /* next 6 bits */
        out[2] = (unsigned char)(0x80 | (cp & 0x3F));          /* low 6 bits */
        return 3;
    }

    /* 4 bytes: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (emoji live here) */
    if (cp <= 0x10FFFF) {
        out[0] = (unsigned char)(0xF0 | (cp >> 18));           /* top 3 bits */
        out[1] = (unsigned char)(0x80 | ((cp >> 12) & 0x3F));  /* next 6 bits */
        out[2] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));   /* next 6 bits */
        out[3] = (unsigned char)(0x80 | (cp & 0x3F));          /* low 6 bits */
        return 4;
    }

    return 0; /* beyond Unicode range */
}

size_t parsi_normalize(const char *in, size_t in_len, char *out, size_t out_cap,
                       unsigned flags)
{
    const unsigned char *s = (const unsigned char *)in;
    size_t i = 0;         /* read position in in */
    size_t need = 0;      /* bytes the full result needs */
    size_t written = 0;   /* bytes actually stored in out */
    int full = 0;         /* set once a character didn't fit */

    while (i < in_len) {
        unsigned char buf[4];
        unsigned long cp;
        size_t used = parsi_utf8_decode(s + i, in_len - i, &cp);
        size_t len, k;

        if (used == 0) {                          /* invalid byte: copy it as is */
            buf[0] = s[i];
            used = len = 1;
        } else {
            if (flags & PARSI_YEH_KAF) {
                if (cp == 0x064A || cp == 0x0649) cp = 0x06CC; /* Arabic yeh, alef maksura */
                else if (cp == 0x0643) cp = 0x06A9;            /* Arabic kaf */
            }
            len = parsi_utf8_encode(cp, buf);     /* cp came from the decoder, so valid */
        }

        if (!full && written + len < out_cap) {   /* keep room for the NUL */
            for (k = 0; k < len; k++) out[written + k] = (char)buf[k];
            written += len;
        } else {
            full = 1;                             /* stop writing, keep counting */
        }

        need += len;
        i += used;
    }

    if (out_cap > 0) out[written] = '\0';
    return need;
}

#endif /* PARSI_IMPLEMENTATION */