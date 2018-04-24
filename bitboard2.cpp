#ifndef _BOTZONE_ONLINE
#include "bitboard2.h"
#endif

#define N(x) ((x) >> 8)
#define S(x) ((x) << 8)
#define W(x) (((x) & 0xfefefefefefefefeull) >> 1)
#define E(x) (((x) & 0x7f7f7f7f7f7f7f7full) << 1)
#define NW(x) (N(W(x)))
#define NE(x) (N(E(x)))
#define SW(x) (S(W(x)))
#define SE(x) (S(E(x)))

u64 getFrontierBB(u64 cur, u64 opp)
{
    u64 empty = getEmpty(cur, opp);
    return (N(empty)
            | S(empty)
            | W(empty)
            | E(empty)
            | NW(empty)
            | NE(empty)
            | SW(empty)
            | SE(empty));
}

#define GETMOVE_HELPER(dir)                               \
    tmp = dir(cur) & opp;                                 \
    for (int i = 0; i < 5; ++i) tmp |= dir(tmp) & opp;    \
    ret |= dir(tmp) & empty;

u64 getMovesBB(u64 cur, u64 opp)
{
    u64 empty = getEmpty(cur, opp);
    u64 tmp, ret = 0;

    GETMOVE_HELPER(N);
    GETMOVE_HELPER(S);
    GETMOVE_HELPER(W);
    GETMOVE_HELPER(E);
    GETMOVE_HELPER(NW);
    GETMOVE_HELPER(NE);
    GETMOVE_HELPER(SW);
    GETMOVE_HELPER(SE);

    return ret;
}

#define GETFLIPPED_HELPER(dir)                                          \
    if (dir(1ull << sq) & opp) {                                        \
        mask = 0;                                                       \
        v = 0;                                                          \
        h = 0;                                                          \
        id = sq + DIR_##dir;                                            \
        tmp = dir(1ull << sq);                                          \
        for (; tmp & opp; id += DIR_##dir, tmp = dir(tmp)) {            \
            mask |= tmp;                                                \
            v += VALUE[id];                                             \
            h ^= PIECE_HASH_VALUE[id][player];                          \
            h ^= PIECE_HASH_VALUE[id][player ^ 1];                      \
        }                                                               \
        if (tmp & cur) {                                                \
            cur ^= mask;                                                \
            opp ^= mask;                                                \
            val += v * 2;                                               \
            hash ^= h;                                                  \
        }                                                               \
    }

void getFlipped(int sq, int player, u64 &cur, u64 &opp, Value &val, HashValue &hash)
{
    u64 mask, tmp;
    int id;
    Value v;
    HashValue h;

    GETFLIPPED_HELPER(N);
    GETFLIPPED_HELPER(S);
    GETFLIPPED_HELPER(W);
    GETFLIPPED_HELPER(E);
    GETFLIPPED_HELPER(NW);
    GETFLIPPED_HELPER(NE);
    GETFLIPPED_HELPER(SW);
    GETFLIPPED_HELPER(SE);
}
