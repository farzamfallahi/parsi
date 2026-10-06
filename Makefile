.RECIPEPREFIX = >
CC     = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -g -fsanitize=address,undefined

test: tests/test_parsi.c parsi.h
> $(CC) $(CFLAGS) -I. tests/test_parsi.c -o tests/test_parsi
> ./tests/test_parsi

clean:
> rm -f tests/test_parsi

.PHONY: test clean
