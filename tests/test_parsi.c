#define PARSI_IMPLEMENTATION
#include "parsi.h"

#include <assert.h>
#include <stdio.h>

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
}

int main(void)
{
    test_decode();
    printf("All tests passed.\n");
    return 0;
}
