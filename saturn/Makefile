# Modeled on the official yaul-org/libyaul-examples/_template/Makefile
ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT. Source ~/.yaul.env before building)
endif

include $(YAUL_INSTALL_ROOT)/share/build.pre.mk

BUILTIN_ASSETS=
SH_PROGRAM:= megaorbit-diag
SH_SRCS:= main.c ../core/megacore.c
SH_CFLAGS+= -O2 -I../core
SH_LDFLAGS+=

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20261020
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= MegaOrbit Diag
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000
IP_1ST_READ_SIZE:= 0

include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk
