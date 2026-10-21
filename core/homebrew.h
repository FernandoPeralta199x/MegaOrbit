#ifndef MEGAORBIT_HOMEBREW_H
#define MEGAORBIT_HOMEBREW_H
#include <stdint.h>
#include <stddef.h>
#define ORBIT_CART_SIZE 1024u
/* Builds original 68000 homebrew test ROM with independent input loop. */
void orbit_cart_build(uint8_t cart[ORBIT_CART_SIZE]);
#endif
