#define PARSI_IMPLEMENTATION
#include "parsi.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define U(s) ((const unsigned char *)(s))

static void test_decode(void)
{
    unsigned long cp;

    /* Valid input */
    assert(parsi_utf8_decode(U("A"), 1, &cp) == 1 && cp == 0x41);
    assert(parsi_utf8_decode(U("\xDA\xA9"), 2, &cp) == 2 && cp == 0x06A9); /* Persian kaf */
    assert(parsi_utf8_decode(U("\xDB\x8C"), 2, &cp) == 2 && cp == 0x06CC); /* Persian yeh */
    assert(parsi_utf8_decode(U("\xD9\x83"), 2, &cp) == 2 && cp == 0x0643); /* Arabic kaf */

    /* Invalid input must return 0, never crash */
    assert(parsi_utf8_decode(U("\xDB"), 1, &cp) == 0);      /* truncated */
    assert(parsi_utf8_decode(U("\x80"), 1, &cp) == 0);      /* lone continuation */
    assert(parsi_utf8_decode(U("\xDB\x41"), 2, &cp) == 0);  /* bad continuation */
    assert(parsi_utf8_decode(U("\xC0\x80"), 2, &cp) == 0);  /* overlong */
    assert(parsi_utf8_decode(U(""), 0, &cp) == 0);          /* empty */

    /* 3-byte sequences */
    assert(parsi_utf8_decode(U("\xE2\x80\x8C"), 3, &cp) == 3 && cp == 0x200C); /* ZWNJ */
    assert(parsi_utf8_decode(U("\xE2\x80"), 2, &cp) == 0);             /* truncated */
    assert(parsi_utf8_decode(U("\xE0\x80\x80"), 3, &cp) == 0);         /* overlong */
    assert(parsi_utf8_decode(U("\xED\xA0\x80"), 3, &cp) == 0);         /* surrogate */

    /* 4-byte sequences */
    assert(parsi_utf8_decode(U("\xF0\x9F\x98\x80"), 4, &cp) == 4 && cp == 0x1F600); /* 😀 */
    assert(parsi_utf8_decode(U("\xF0\x9F\x98"), 3, &cp) == 0);         /* truncated */
    assert(parsi_utf8_decode(U("\xF0\x80\x80\x80"), 4, &cp) == 0);     /* overlong */
    assert(parsi_utf8_decode(U("\xF4\x90\x80\x80"), 4, &cp) == 0);     /* above U+10FFFF */
    assert(parsi_utf8_decode(U("\xF8\x88\x80\x80\x80"), 5, &cp) == 0); /* 5-byte lead */
}

static void test_encode(void)
{
    unsigned char out[4];

    /* Known byte sequences, one per length */
    assert(parsi_utf8_encode(0x41, out) == 1 && memcmp(out, "A", 1) == 0);
    assert(parsi_utf8_encode(0x06A9, out) == 2 && memcmp(out, "\xDA\xA9", 2) == 0);         /* Persian kaf */
    assert(parsi_utf8_encode(0x200C, out) == 3 && memcmp(out, "\xE2\x80\x8C", 3) == 0);     /* ZWNJ */
    assert(parsi_utf8_encode(0x1F600, out) == 4 && memcmp(out, "\xF0\x9F\x98\x80", 4) == 0); /* emoji */

    /* Invalid code points must return 0 */
    assert(parsi_utf8_encode(0xD800, out) == 0);   /* first surrogate */
    assert(parsi_utf8_encode(0xDFFF, out) == 0);   /* last surrogate */
    assert(parsi_utf8_encode(0x110000, out) == 0); /* above U+10FFFF */
}

static void test_roundtrip(void)
{
    static const unsigned long cps[] = {
        0x41,     /* ASCII 'A' */
        0x06A9,   /* Persian kaf */
        0x06CC,   /* Persian yeh */
        0x200C,   /* ZWNJ */
        0x1F600,  /* emoji */
    };
    size_t i;

    for (i = 0; i < sizeof cps / sizeof cps[0]; i++) {
        unsigned char out[4];
        unsigned long cp;
        size_t n = parsi_utf8_encode(cps[i], out);

        assert(n > 0);
        assert(parsi_utf8_decode(out, n, &cp) == n && cp == cps[i]);
    }
}

/* UTF-8 spellings used below */
#define AR_KAF     "\xD9\x83"  /* U+0643 Arabic kaf */
#define AR_YEH     "\xD9\x8A"  /* U+064A Arabic yeh */
#define AR_MAKSURA "\xD9\x89"  /* U+0649 Arabic alef maksura */
#define FA_KAF     "\xDA\xA9"  /* U+06A9 Persian kaf */
#define FA_YEH     "\xDB\x8C"  /* U+06CC Persian yeh */
#define TEH        "\xD8\xAA"  /* U+062A */
#define ALEF       "\xD8\xA7"  /* U+0627 */
#define BEH        "\xD8\xA8"  /* U+0628 */
#define ZWNJ       "\xE2\x80\x8C"

/* Normalize in (a string literal) with flags, and check the result is
 * exactly want, NUL-terminated, with the full length returned. */
static void check_normalize(const char *in, unsigned flags, const char *want)
{
    char out[64];
    size_t want_len = strlen(want);
    size_t n = parsi_normalize(in, strlen(in), out, sizeof out, flags);

    assert(n == want_len);
    assert(memcmp(out, want, want_len) == 0 && out[want_len] == '\0');
}

static void test_normalize(void)
{
    char out[16];

    /* کتاب: Arabic spelling becomes Persian, Persian stays as is */
    check_normalize(AR_KAF TEH ALEF BEH, PARSI_YEH_KAF, FA_KAF TEH ALEF BEH);
    check_normalize(FA_KAF TEH ALEF BEH, PARSI_YEH_KAF, FA_KAF TEH ALEF BEH);

    /* یک: Arabic yeh or alef maksura plus Arabic kaf become Persian */
    check_normalize(AR_YEH AR_KAF,     PARSI_YEH_KAF, FA_YEH FA_KAF);
    check_normalize(AR_MAKSURA AR_KAF, PARSI_YEH_KAF, FA_YEH FA_KAF);
    check_normalize(FA_YEH FA_KAF,     PARSI_YEH_KAF, FA_YEH FA_KAF);

    /* Without the flag nothing changes */
    check_normalize(AR_YEH AR_KAF, 0, AR_YEH AR_KAF);

    /* ASCII, ZWNJ and emoji pass through */
    check_normalize("abc " AR_KAF ZWNJ "\xF0\x9F\x98\x80", PARSI_YEH_KAF,
                    "abc " FA_KAF ZWNJ "\xF0\x9F\x98\x80");

    /* Invalid UTF-8 bytes are copied through unchanged */
    check_normalize("\xFF" AR_KAF "\x80", PARSI_YEH_KAF, "\xFF" FA_KAF "\x80");
    check_normalize(AR_KAF "\xD9", PARSI_YEH_KAF, FA_KAF "\xD9");  /* truncated at end */
    check_normalize("\xED\xA0\x80", PARSI_YEH_KAF, "\xED\xA0\x80"); /* surrogate */

    /* Empty input */
    assert(parsi_normalize("", 0, out, sizeof out, PARSI_YEH_KAF) == 0 && out[0] == '\0');

    /* Like snprintf: the full length is returned even when out is too small */
    assert(parsi_normalize(AR_KAF TEH ALEF BEH, 8, NULL, 0, PARSI_YEH_KAF) == 8);

    /* Exact fit: 8 bytes plus the NUL */
    memset(out, 'x', sizeof out);
    assert(parsi_normalize(AR_KAF TEH ALEF BEH, 8, out, 9, PARSI_YEH_KAF) == 8);
    assert(memcmp(out, FA_KAF TEH ALEF BEH, 8) == 0 && out[8] == '\0');

    /* One byte short: the last character is dropped, never split */
    memset(out, 'x', sizeof out);
    assert(parsi_normalize(AR_KAF TEH ALEF BEH, 8, out, 8, PARSI_YEH_KAF) == 8);
    assert(memcmp(out, FA_KAF TEH ALEF, 6) == 0 && out[6] == '\0' && out[7] == 'x');

    /* Room for 3 bytes: only kaf fits, TEH would be split */
    memset(out, 'x', sizeof out);
    assert(parsi_normalize(AR_KAF TEH ALEF BEH, 8, out, 4, PARSI_YEH_KAF) == 8);
    assert(memcmp(out, FA_KAF, 2) == 0 && out[2] == '\0' && out[3] == 'x');

    /* out_cap 1: just the NUL */
    assert(parsi_normalize(AR_KAF, 2, out, 1, PARSI_YEH_KAF) == 2 && out[0] == '\0');
}

int main(void)
{
    test_decode();
    test_encode();
    test_roundtrip();
    test_normalize();
    printf("All tests passed.\n");
    return 0;
}
