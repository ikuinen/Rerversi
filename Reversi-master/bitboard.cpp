#include "bitboard.h"

const int DIR_N = -8;
const int DIR_S = +8;
const int DIR_W = -1;
const int DIR_E = +1;
const int DIR_NW = DIR_N + DIR_W;
const int DIR_NE = DIR_N + DIR_E;
const int DIR_SW = DIR_S + DIR_W;
const int DIR_SE = DIR_S + DIR_E;

#define N(x) ((x) >> 8)
#define S(x) ((x) << 8)
#define W(x) (((x) & 0xfefefefefefefefeull) >> 1)
#define E(x) (((x) & 0x7f7f7f7f7f7f7f7full) << 1)
#define NW(x) (N(W(x)))
#define NE(x) (N(E(x)))
#define SW(x) (S(W(x)))
#define SE(x) (S(E(x)))

#define GETMOVE_HELPER(dir)                               \
    tmp = dir(own) & enemy;                                 \
    for (int i = 0; i < 5; ++i) tmp |= dir(tmp) & enemy;    \
    ret |= dir(tmp) & empty;

uint64 find_correct_moves(uint64 own, uint64 enemy)
{
	uint64 empty = ~(own | enemy);
	uint64 tmp, ret = 0;
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
    if (dir(1ULL << action) & enemy) {                                        \
        mask = 0;                                                       \
        v = 0;                                                          \
        h = 0;                                                          \
        id = action + DIR_##dir;                                            \
        tmp = dir(1ULL << action);                                          \
        for (; tmp & enemy; id += DIR_##dir, tmp = dir(tmp)) {            \
            mask |= tmp;                                                \
            v += V[id >> 3][id & 0x7];                                             \
            h ^= HASH_VALUE[id][player];                          \
            h ^= HASH_VALUE[id][player ^ 1];                      \
        }                                                               \
        if (tmp & own) {                                                \
            own ^= mask;                                                \
            enemy ^= mask;                                                \
            val += v * 2;                                               \
            hashCode ^= h;                                                  \
        }                                                               \
    }

void do_flip(int action, uint64 &own, uint64 &enemy, int &val, uint64 &hashCode, int player)
{
	uint64 mask, tmp;
	int id;
	int v;
	uint64 h;

	GETFLIPPED_HELPER(N);
	GETFLIPPED_HELPER(S);
	GETFLIPPED_HELPER(W);
	GETFLIPPED_HELPER(E);
	GETFLIPPED_HELPER(NW);
	GETFLIPPED_HELPER(NE);
	GETFLIPPED_HELPER(SW);
	GETFLIPPED_HELPER(SE);
}

int find_correct_moves_array(uint64 own, uint64 enemy, int * dest, bool random)
{
	uint64 legal_moves = find_correct_moves(own, enemy);
	int len = 0;
	for (int i = 0; i < 64; i++) {
		if (legal_moves & 1)
			dest[len++] = i;
		legal_moves >>= 1;
	}
	//srand(time(0));
	if (random) {
		int t;
		for (int i = 0; i < len; i++) {
			int index = rand() % (len - i) + i;
			t = dest[index];
			dest[index] = dest[i];
			dest[i] = t;
		}
	}
	dest[len] = -1;
	return len;
}

int bit_to_array(uint64 x, int * dest)
{
	int len = 0;
	for (int i = 0; i < 64; i++) {
		if (x & 1)
			dest[len++] = i;
		x >>= 1;
	}
	dest[len] = -1;
	return len;
}

uint64 calc_flip(int pos, uint64 own, uint64 enemy)
{
	uint64 f1 = calc_flip_half(pos, own, enemy), f2 = calc_flip_half(63 - pos, rotate180(own), rotate180(enemy));
	return f1 | rotate180(f2);
}

uint64 calc_flip_half(int pos, uint64 own, uint64 enemy)
{
	const uint64 el[] = {
		enemy, enemy & 0x7e7e7e7e7e7e7e7eULL,
		enemy & 0x7e7e7e7e7e7e7e7eULL, enemy & 0x7e7e7e7e7e7e7e7eULL
	};

	const uint64 masks[] = {
		0x0101010101010100ULL << pos, 0x00000000000000feULL << pos,
		0x0002040810204080ULL << pos, 0x8040201008040200ULL << pos
	};

	uint64 flipped = 0ULL;
	for (int i = 0; i < 4; i++) {
		uint64 outflank = masks[i] & ((el[i] | ~masks[i]) + 1ULL) & own;
		flipped |= (outflank - (uint64)(outflank != 0ULL)) & masks[i];
	}
	return flipped;
}

int bit_count(uint64 x)
{
	int count = 0;
	while (x) {
		if (x & 1ULL)
			count++;
		x >>= 1;
	}
	return count;
}

uint64 getFrontier(uint64 own, uint64 enemy)
{
	uint64 empty = ~(own | enemy);
	return (N(empty)
		| S(empty)
		| W(empty)
		| E(empty)
		| NW(empty)
		| NE(empty)
		| SW(empty)
		| SE(empty));
}

uint64 flip_diag_a1h8(uint64 x)
{
	uint64 k1 = 0x5500550055005500ULL, k2 = 0x3333000033330000ULL, k4 = 0x0f0f0f0f00000000ULL;
	uint64 t;
	t = k4 & (x ^ (x << 28));
	x ^= t ^ (t >> 28);
	t = k2 & (x ^ (x << 14));
	x ^= t ^ (t >> 14);
	t = k1 & (x ^ (x << 7));
	x ^= t ^ (t >> 7);
	return x;
}

uint64 rotate90(uint64 x)
{
	return flip_diag_a1h8(flip_vertical(x));
}

uint64 rotate180(uint64 x)
{
	return rotate90(rotate90(x));
}

uint64 flip_vertical(uint64 x)
{
	uint64 k1 = 0x00FF00FF00FF00FFULL;
	uint64 k2 = 0x0000FFFF0000FFFFULL;
	x = ((x >> 8) & k1) | ((x & k1) << 8);
	x = ((x >> 16) & k2) | ((x & k2) << 16);
	x = (x >> 32) | (x << 32);
	return x;
}

uint64 search_offset_right(uint64 own, uint64 enemy, uint64 mask, int offset)
{
	uint64 e = enemy & mask;
	uint64 blank = ~(own | enemy);
	uint64 t;
	t = e & (own << offset);
	t |= e & (t << offset);
	t |= e & (t << offset);
	t |= e & (t << offset);
	t |= e & (t << offset);
	t |= e & (t << offset);  //Up to six stones can be turned at once
	return blank & (t << offset);  // Only the blank squares can be started
}

uint64 search_offset_left(uint64 own, uint64 enemy, uint64 mask, int offset)
{
	uint64 e = enemy & mask;
	uint64 blank = ~(own | enemy);
	uint64 t;
	t = e & (own >> offset);
	t |= e & (t >> offset);
	t |= e & (t >> offset);
	t |= e & (t >> offset);
	t |= e & (t >> offset);
	t |= e & (t >> offset);  //Up to six stones can be turned at once
	return blank & (t >> offset);  // Only the blank squares can be started
}