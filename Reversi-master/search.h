#ifndef SEARCH_H_
#define SEARCH_H_

#ifndef _BOTZONE_ONLINE
#include "constant.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <string>
#include "bitboard2.h"
#include "position.h"
#include "constant.h"
#endif



bool isTimeUp();
void initHashTable();
SVPair probeHash(int depth, Value alpha, Value beta, const Position &pos, bool &cut);
void recordHash(const Position &pos, Value value, int depth, HashType type, int bestMove);
SVPair alphabeta(int depth, Value alpha, Value beta, const Position &pos, bool requireMove = false);
SVPair getBestMove(const Position &pos, int &maxdepth);


#endif