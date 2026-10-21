CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -Werror -pedantic

.PHONY: all test clean
all: test

build/test_core: core/megacore.c core/megacore.h tests/test_core.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ core/megacore.c tests/test_core.c

test: build/test_core
	./build/test_core

clean:
	rm -rf build
