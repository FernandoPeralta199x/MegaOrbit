CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -Werror -pedantic
.PHONY: all test clean
all: test
build/test_core: core/megacore.c core/megacore.h tests/test_core.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ core/megacore.c tests/test_core.c
build/test_hardware: core/md_emulation.c core/md_emulation.h core/md_vdp.c core/md_vdp.h tests/test_hardware.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ core/md_emulation.c core/md_vdp.c tests/test_hardware.c
test: build/test_core build/test_hardware
	./build/test_core
	./build/test_hardware
clean:
	rm -rf build
