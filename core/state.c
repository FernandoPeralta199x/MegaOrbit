#include "state.h"
#include <string.h>
#define STATE_MAGIC 0x4D4F5354u
static uint32_t fnv(uint32_t h,const void *data,size_t len){
 const uint8_t *p=(const uint8_t *)data;
 size_t i;
 for(i=0;i<len;i++){h^=p[i];h*=16777619u;}
 return h;
}
static uint32_t rom_hash(const md_bus_t *bus){
 uint32_t hash=2166136261u;
 size_t n=bus->rom_size;
 if(bus->rom!=NULL&&n)hash=fnv(hash,bus->rom,n);
 return hash;
}
static uint32_t snapshot_hash(const md_state_t *s){
 uint32_t h=2166136261u;
 h=fnv(h,&s->magic,sizeof(s->magic));
 h=fnv(h,&s->version,sizeof(s->version));
 h=fnv(h,&s->rom_size,sizeof(s->rom_size));
 h=fnv(h,&s->rom_fingerprint,sizeof(s->rom_fingerprint));
 h=fnv(h,&s->cpu,sizeof(s->cpu));
 h=fnv(h,s->ram,sizeof(s->ram));
 h=fnv(h,s->io,sizeof(s->io));
 h=fnv(h,s->tmss,sizeof(s->tmss));
 return h;
}
int md_state_save(md_state_t *s,const m68k_t *cpu,const md_bus_t *bus){
 if(s==NULL||cpu==NULL||bus==NULL||bus->rom==NULL||
   bus->rom_size==0u||bus->rom_size>0xFFFFFFFFu)return 0;
 memset(s,0,sizeof(*s));
 s->magic=STATE_MAGIC;
 s->version=MEGAORBIT_STATE_VERSION;
 s->rom_size=(uint32_t)bus->rom_size;
 s->rom_fingerprint=rom_hash(bus);
 s->cpu=*cpu;
 memcpy(s->ram,bus->ram,MD_RAM_SIZE);
 memcpy(s->io,bus->io,sizeof(s->io));
 memcpy(s->tmss,bus->tmss,sizeof(s->tmss));
 s->fingerprint=snapshot_hash(s);
 return 1;
}
int md_state_load(const md_state_t *s,m68k_t *cpu,md_bus_t *bus){
 /* All-or-nothing validation before restoring any state. */
 if(s==NULL||cpu==NULL||bus==NULL||
    s->magic!=STATE_MAGIC||s->version!=MEGAORBIT_STATE_VERSION||
    s->rom_size!=bus->rom_size||s->rom_fingerprint!=rom_hash(bus)||
    s->fingerprint!=snapshot_hash(s))return 0;
 *cpu=s->cpu;
 memcpy(bus->ram,s->ram,MD_RAM_SIZE);
 memcpy(bus->io,s->io,sizeof(s->io));
 memcpy(bus->tmss,s->tmss,sizeof(s->tmss));
 return 1;
}
