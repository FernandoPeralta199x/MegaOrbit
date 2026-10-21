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
build/test_homebrew: core/megacore.c core/homebrew.c core/homebrew.h tests/test_homebrew.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ core/megacore.c core/homebrew.c tests/test_homebrew.c

build/test_state: core/megacore.c core/state.c core/homebrew.c tests/test_state.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ core/megacore.c core/state.c core/homebrew.c tests/test_state.c

build/test_menu: ui/menu.c ui/menu.h tests/test_menu.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ ui/menu.c tests/test_menu.c

test: build/test_core build/test_hardware build/test_homebrew build/test_state build/test_menu
	./build/test_core
	./build/test_hardware
	./build/test_homebrew
	./build/test_state
	./build/test_menu
clean:
	rm -rf build
