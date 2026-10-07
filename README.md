# parsi
Persian (Farsi) text normalisation in C, as a single-header library.

Persian text often mixes in Arabic code points that look the same but
compare differently, such as Arabic kaf `ك` (U+0643) instead of Persian
kaf `ک` (U+06A9). `parsi` rewrites them to their Persian forms so that
searching, sorting and comparing work. It can also convert digits
between ASCII, Arabic-Indic and Persian forms. It also has a small, strict UTF-8
decoder and encoder. It is plain C99 with no dependencies.

## Including it

Copy `parsi.h` into your project. In **one** `.c` file, define
`PARSI_IMPLEMENTATION` before including it to compile the implementation:

```c
#define PARSI_IMPLEMENTATION
#include "parsi.h"
```

Every other file just includes the header:

```c
#include "parsi.h"
```

## Usage

`parsi_normalize` works like `snprintf`: it returns the length the full
result needs (not counting the NUL), so you can call it with `NULL` and `0`
to measure first, then allocate exactly enough.

```c
#define PARSI_IMPLEMENTATION
#include "parsi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    const char *in = "\xD9\x8A\xD9\x83";   /* "يك" — Arabic yeh + Arabic kaf */
    size_t in_len = strlen(in);

    /* 1. Ask how many bytes the result needs */
    size_t need = parsi_normalize(in, in_len, NULL, 0, PARSI_YEH_KAF);

    /* 2. Allocate that plus one for the NUL, and normalize */
    char *out = malloc(need + 1);
    if (!out) return 1;
    parsi_normalize(in, in_len, out, need + 1, PARSI_YEH_KAF);

    printf("%s\n", out);                   /* prints "یک" — Persian yeh + kaf */
    free(out);
    return 0;
}
```

With a fixed-size buffer, a return value `>= out_cap` means the output was
truncated. Truncation never splits a character, and the output is always
NUL-terminated when `out_cap > 0`. Bytes that are not valid UTF-8 are
copied through unchanged.

### Flags

Combine flags with `|`, for example `PARSI_YEH_KAF | PARSI_DIGITS_TO_EN`.

| Flag                 | Effect                                                              |
|----------------------|---------------------------------------------------------------------|
| `PARSI_YEH_KAF`      | `ي` U+064A and `ى` U+0649 → `ی` U+06CC; `ك` U+0643 → `ک` U+06A9      |
| `PARSI_DIGITS_TO_FA` | ASCII `0`–`9` and Arabic-Indic `٠`–`٩` (U+0660–0669) → Persian `۰`–`۹` (U+06F0–06F9) |
| `PARSI_DIGITS_TO_EN` | Persian `۰`–`۹` and Arabic-Indic `٠`–`٩` → ASCII `0`–`9`              |

If both digit flags are set, `PARSI_DIGITS_TO_EN` wins: ASCII digits are
what `atoi`, `strtol` and most parsers expect.

The output can be longer than the input. `PARSI_DIGITS_TO_FA` turns each
1-byte ASCII digit into a 2-byte Persian one, so size the buffer from the
return value, not from `in_len`.

### UTF-8 helpers

```c
size_t parsi_utf8_decode(const unsigned char *s, size_t len, unsigned long *cp);
size_t parsi_utf8_encode(unsigned long cp, unsigned char *out); /* out: 4 bytes */
```

Both return the number of bytes used (1–4), or 0 for invalid input:
truncated or overlong sequences, surrogates (U+D800–U+DFFF) or code points
above U+10FFFF.

## Running the tests

```sh
make test
```

This builds `tests/test_parsi.c` with `-Wall -Wextra -Wpedantic` and
AddressSanitizer/UBSan, then runs it. `make clean` removes the binary.
