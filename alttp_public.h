#ifndef __ALTTP_PUBLIC_H__
#define __ALTTP_PUBLIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

struct ap_snes9x {
    uint8_t * (*base)(uint32_t addr);
    int (*save)(const char * filename);
    int (*load)(const char * filename);
    const char ** info_string_ptr;
};

void ap_init(struct ap_snes9x * emu);
void ap_tick(uint32_t frame, uint16_t * joypad);

#ifdef __cplusplus
}
#endif

#endif
