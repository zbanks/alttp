#include "ap_snes.h"
#include "ap_macro.h"

struct ap_ram ap_ram;
struct ap_snes9x * ap_emu = NULL;

char ap_info_string[INFO_STRING_SIZE];
bool ap_debug;

void
ap_init(struct ap_snes9x * _emu)
{
    ap_emu = _emu;

#define X(name, type, offset) ap_ram.name = (type *) ap_emu->base(offset);
AP_RAM_LIST
#undef X

    printf("----- initialized ------ \n");
    ap_emu->load("last");
}
