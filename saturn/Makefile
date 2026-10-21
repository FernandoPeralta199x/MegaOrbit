ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT; configure Yaul SDK)
endif
include $(YAUL_INSTALL_ROOT)/share/build.pre.mk
BUILTIN_ASSETS+= assets/sonic_slot.bin;asset_sonic_slot
SH_PROGRAM:= megaorbit-demo
SH_SRCS:= main.c scsp_audio.c ui_background.c ../core/homebrew.c ../core/megacore.c ../core/md_emulation.c ../core/md_vdp.c ../ui/menu.c
SH_CFLAGS+= -Os -I. -I../core -I../ui
SH_LDFLAGS+=
IP_VERSION:= V0.500
IP_RELEASE_DATE:= 20261020
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= MegaOrbit GriffonBR
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000
IP_1ST_READ_SIZE:= 0
include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk
