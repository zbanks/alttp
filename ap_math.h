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
#define XYFN1(fn, a) XY(fn((a).x), fn((a).y))
#define XYFN2(fn, a, b) XY(fn((a).x, (b).x), fn((a).y, (b).y))
#define XYL1DIST(a, b) ((int) ABSDIFF((a).x, (b).x) + (int) ABSDIFF((a).y, (b).y))
#define XYMAP8(xy) ((((xy).x & 0x1F8) >> 3) | (((xy).y) & 0x1F8) << 3)
#define XYMAPSCREEN(xy) ( ((xy).x >> 8) | ((xy).y & 0xFF00) )
#define XYMID(a, b) XYOP2(XYOP1(a, / 2), +, XYOP1(b, / 2))
#define XYIN(xy, tl, br) ((xy).x >= (tl).x && (xy).y >= (tl).y && (xy).x <= (br).x && (xy).y <= (br).y)
#define XYLINKIN(xy, tl, br) ((xy).x >= (tl).x && (xy).y >= (tl).y && (xy).x + 15 <= (br).x && (xy).y + 15 <= (br).y)
#define XYEQ(a, b) ((a).x == (b).x && (a).y == (b).y)
#define XYRDIV8(xy) XY(((xy).x + 3) / 8, ((xy).y + 3) / 8)
#define XYUNDER(xy, br) ((xy).x < (br).x && (xy).y < (br).y)

#define XYINDOORS(xy) ((xy).x >= 0x4000)
#define XYONLOWER(xy) (XYINDOORS(xy) && ((xy).x & 0x200) == 0x000)
#define XYONUPPER(xy) (XYINDOORS(xy) && ((xy).x & 0x200) == 0x200)
#define XYFLIPBG(xy) XY((xy).x ^ 0x200, (xy).y)
#define XYTOLOWER(xy) XY((xy).x & ~0x200, (xy).y)
#define XYTOUPPER(xy) XY((xy).x | 0x200, (xy).y)

#define PRIXYV "(%u %#06x, %u %#06x)"
#define PRIXYVF(xy) (xy).x, (xy).x, (xy).y, (xy).y
#define PRIXY "%x,%x"
#define PRIXYF(xy) (xy).x, (xy).y
#define PRIBB PRIXY ";" PRIXY
#define PRIBBF(bb) PRIXYF((bb).tl), PRIXYF((bb).br)
#define PRIBBV PRIXYV " x " PRIXYV
#define PRIBBVF(bb) PRIXYVF((bb).tl), PRIXYVF((bb).br)
#define PRIBBWH PRIXY "@" PRIXY
#define PRIBBWHF(bb) PRIXYF(XYOP2((bb).br, -, (bb).tl)), PRIXYF((bb).tl)

static inline int
XYL1BOXDIST(struct xy xy, struct xy tl, struct xy br) {
    int dist = 0;
    if (tl.x > xy.x) {
        dist += tl.x - xy.x;
    } else if (xy.x > br.x) {
        dist += xy.x - br.x;
    }
    if (tl.y > xy.y) {
        dist += tl.y - xy.y;
    } else if (xy.y > br.y) {
        dist += xy.y - br.y;
    }
    return dist;
}
