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

/* Persian digits U+06F0-06F9 and Arabic-Indic digits U+0660-0669 */
#define FA_DIGITS "\xDB\xB0\xDB\xB1\xDB\xB2\xDB\xB3\xDB\xB4\xDB\xB5\xDB\xB6\xDB\xB7\xDB\xB8\xDB\xB9"
#define AR_DIGITS "\xD9\xA0\xD9\xA1\xD9\xA2\xD9\xA3\xD9\xA4\xD9\xA5\xD9\xA6\xD9\xA7\xD9\xA8\xD9\xA9"
#define FA_0 "\xDB\xB0"
#define FA_1 "\xDB\xB1"
#define FA_2 "\xDB\xB2"
#define FA_3 "\xDB\xB3"
#define FA_4 "\xDB\xB4"
#define AR_3 "\xD9\xA3"

static void test_normalize_digits(void)
{
    char out[16];

    /* PARSI_DIGITS_TO_FA: ASCII and Arabic-Indic become Persian */
    check_normalize("0123456789", PARSI_DIGITS_TO_FA, FA_DIGITS);
    check_normalize(AR_DIGITS,    PARSI_DIGITS_TO_FA, FA_DIGITS);
    check_normalize(FA_DIGITS,    PARSI_DIGITS_TO_FA, FA_DIGITS);

    /* PARSI_DIGITS_TO_EN: Persian and Arabic-Indic become ASCII */
    check_normalize(FA_DIGITS,    PARSI_DIGITS_TO_EN, "0123456789");
    check_normalize(AR_DIGITS,    PARSI_DIGITS_TO_EN, "0123456789");
    check_normalize("0123456789", PARSI_DIGITS_TO_EN, "0123456789");

    /* Neighbours of the digit ranges are left alone */
    check_normalize("/:", PARSI_DIGITS_TO_FA, "/:");                       /* U+002F, U+003A */
    check_normalize("\xDB\xAF\xDB\xBA", PARSI_DIGITS_TO_EN, "\xDB\xAF\xDB\xBA"); /* U+06EF, U+06FA */
    check_normalize("\xD9\x9F\xD9\xAA", PARSI_DIGITS_TO_EN, "\xD9\x9F\xD9\xAA"); /* U+065F, U+066A */

    /* Without a digit flag, digits are unchanged */
    check_normalize("1" FA_2 AR_3, PARSI_YEH_KAF, "1" FA_2 AR_3);

    /* Mixed string: Arabic letters, a space, and all three digit kinds */
    check_normalize(AR_YEH AR_KAF " 1" FA_2 AR_3, PARSI_YEH_KAF | PARSI_DIGITS_TO_FA,
                    FA_YEH FA_KAF " " FA_1 FA_2 FA_3);
    check_normalize(AR_YEH AR_KAF " 1" FA_2 AR_3, PARSI_YEH_KAF | PARSI_DIGITS_TO_EN,
                    FA_YEH FA_KAF " 123");
    check_normalize(AR_YEH AR_KAF " 1" FA_2 AR_3, PARSI_DIGITS_TO_EN,
                    AR_YEH AR_KAF " 123");

    /* Both digit flags: PARSI_DIGITS_TO_EN wins */
    check_normalize("1" FA_2 AR_3, PARSI_DIGITS_TO_FA | PARSI_DIGITS_TO_EN, "123");

    /* Output longer than input: "2024" is 4 bytes in, 8 bytes out */
    assert(parsi_normalize("2024", 4, NULL, 0, PARSI_DIGITS_TO_FA) == 8);

    memset(out, 'x', sizeof out);
    assert(parsi_normalize("2024", 4, out, 9, PARSI_DIGITS_TO_FA) == 8);   /* exact fit */
    assert(memcmp(out, FA_2 FA_0 FA_2 FA_4, 8) == 0 && out[8] == '\0');

    memset(out, 'x', sizeof out);
    assert(parsi_normalize("2024", 4, out, 5, PARSI_DIGITS_TO_FA) == 8);   /* in_len + 1 is too small */
    assert(memcmp(out, FA_2 FA_0, 4) == 0 && out[4] == '\0' && out[5] == 'x');

    /* Output shorter than input: 8 bytes in, 4 bytes out */
    assert(parsi_normalize(FA_2 FA_0 FA_2 FA_4, 8, out, sizeof out, PARSI_DIGITS_TO_EN) == 4);
    assert(strcmp(out, "2024") == 0);
}

/* Arabic diacritics U+064B-0652 and superscript alef U+0670 */
#define FATHATAN "\xD9\x8B"
#define DAMMATAN "\xD9\x8C"
#define KASRATAN "\xD9\x8D"
#define FATHA    "\xD9\x8E"
#define DAMMA    "\xD9\x8F"
#define KASRA    "\xD9\x90"
#define SHADDA   "\xD9\x91"
#define SUKUN    "\xD9\x92"
#define SUP_ALEF "\xD9\xB0"

static void test_normalize_diacritics(void)
{
    char out[16];

    /* Every diacritic in the range is removed */
    check_normalize(FATHATAN DAMMATAN KASRATAN FATHA DAMMA KASRA SHADDA SUKUN SUP_ALEF,
                    PARSI_DIACRITICS, "");

    /* کِتاب with a kasra, and Arabic كِتَاب with kaf mapped too */
    check_normalize(FA_KAF KASRA TEH ALEF BEH, PARSI_DIACRITICS, FA_KAF TEH ALEF BEH);
    check_normalize(AR_KAF KASRA TEH FATHA ALEF BEH, PARSI_DIACRITICS | PARSI_YEH_KAF,
                    FA_KAF TEH ALEF BEH);

    /* Without the flag, diacritics are kept */
    check_normalize(FA_KAF KASRA TEH, PARSI_YEH_KAF, FA_KAF KASRA TEH);

    /* Neighbours of the ranges are kept: U+064A yeh, U+0653 maddah,
     * U+066F dotless qaf, U+0671 alef wasla */
    check_normalize("\xD9\x8A\xD9\x93\xD9\xAF\xD9\xB1", PARSI_DIACRITICS,
                    "\xD9\x8A\xD9\x93\xD9\xAF\xD9\xB1");

    /* Mixed with ASCII, ZWNJ and other flags */
    check_normalize("a" SHADDA "1" ZWNJ AR_YEH SUKUN, PARSI_DIACRITICS | PARSI_YEH_KAF | PARSI_DIGITS_TO_FA,
                    "a" FA_1 ZWNJ FA_YEH);

    /* Removed characters take no space: 10 bytes in, 4 bytes out */
    assert(parsi_normalize(FA_KAF FATHA SHADDA TEH SUKUN, 10, NULL, 0, PARSI_DIACRITICS) == 4);

    memset(out, 'x', sizeof out);
    assert(parsi_normalize(FA_KAF FATHA SHADDA TEH SUKUN, 10, out, 5, PARSI_DIACRITICS) == 4);
    assert(memcmp(out, FA_KAF TEH, 4) == 0 && out[4] == '\0' && out[5] == 'x');

    /* Truncated output still skips diacritics after the cut */
    memset(out, 'x', sizeof out);
    assert(parsi_normalize(FA_KAF FATHA TEH SUKUN, 8, out, 3, PARSI_DIACRITICS) == 4);
    assert(memcmp(out, FA_KAF, 2) == 0 && out[2] == '\0' && out[3] == 'x');

    /* Only diacritics: empty result */
    assert(parsi_normalize(FATHA KASRA, 4, out, sizeof out, PARSI_DIACRITICS) == 0 && out[0] == '\0');
}

int main(void)
{
    test_decode();
    test_encode();
    test_roundtrip();
    test_normalize();
    test_normalize_digits();
    test_normalize_diacritics();
    printf("All tests passed.\n");
    return 0;
}
