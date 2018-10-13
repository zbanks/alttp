#pragma once
#include <ap_macro.h>

#define ABSDIFF(x, y) (((x) < (y)) ? ((y) - (x)) : ((x) - (y)))
#define ABS(x)    ((x) < 0 ? -(x) : (x))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define CAST16(x) ((uint16_t) (x))

struct xy {
    uint16_t x;
    uint16_t y;
};

#define XY(_x, _y) ((struct xy) { .x = CAST16(_x), .y = CAST16(_y) })
#define XYOP1(a, op) XY(((a).x op), ((a).y op))
#define XYOP2(a, op, b) XY(((a).x op (b).x), ((a).y op (b).y))
#define XYL1DIST(a, b) ((int) ABSDIFF((a).x, (b).x) + (int) ABSDIFF((a).y, (b).y))
#define XYMAP8(xy) ((((xy).x & 0x1F8) >> 3) | (((xy).y) & 0x1F8) << 3)
#define XYMAPNODE(xy) ( ((xy).x >> 8) | ((xy).y & 0xFF00) )
#define XYMID(a, b) XYOP2(XYOP1(a, / 2), +, XYOP1(b, / 2))
#define XYIN(xy, tl, br) ((xy).x >= (tl).x && (xy).y >= (tl).y && (xy).x <= (br).x && (xy).y <= (br).y)

#define PRIXYV "(%u %#x, %u %#x)"
#define PRIXYVF(xy) (xy).x, (xy).x, (xy).y, (xy).y
#define PRIXY "%x,%x"
#define PRIXYF(xy) (xy).x, (xy).y

