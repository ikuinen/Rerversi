#ifndef _BOTZONE_ONLINE
#include "search.h"
#endif

HashNode hashTable[HASH_TABLE_SIZE + 1];
clock_t startTime;

bool isTimeUp()
{
	return (clock() - startTime) > 1.98 * CLOCKS_PER_SEC;
}

void initHashTable()
{
	memset(hashTable, 0, sizeof(hashTable));
}

SVPair probeHash(int depth, Value alpha, Value beta, const Position &pos, bool &cut)
{
	HashValue key = pos.getHashValue();
	int idx = key & HASH_TABLE_MASK;
	HashNode &node = hashTable[idx];

	if (node.key != key) 
		return SVPair(-1, UNKNOWN_VALUE);

	if (node.depth >= depth) {
		cut = true;
		if (node.type == EXACT) {
			return SVPair(node.bestMove, node.value);
		}
		else if (node.type == ALPHA) {
			if (node.value <= alpha)
				return SVPair(node.bestMove, alpha);
		}
		else {
			assert(node.type == BETA);
			if (node.value >= beta)
				return SVPair(node.bestMove, beta);
		}
		cut = false;
	}

	return SVPair(node.bestMove, node.value);
}

void recordHash(const Position &pos, Value value, int depth, HashType type, int bestMove)
{
	HashValue key = pos.getHashValue();
	int idx = key & HASH_TABLE_MASK;
	HashNode &node = hashTable[idx];

	if (node.depth <= depth) {
		node.key = key;
		node.value = value;
		node.depth = depth;
		node.type = type;
		node.bestMove = bestMove;
	}
}

SVPair alphabeta(int depth, Value alpha, Value beta, const Position &pos, bool requireMove)
{
	if (pos.isGameEnd()) {
		return SVPair(-1, pos.getGameEndEval());
	}

	bool cut = false;
	SVPair lastsv = probeHash(depth, alpha, beta, pos, cut);
	if (cut && (!requireMove || lastsv.first != -1))
		return lastsv;

	if (depth == 0) {
		Value val = pos.getEval();
		recordHash(pos, val, depth, EXACT, -1);
		return SVPair(-1, val);
	}

#ifndef FIXED_DEPTH
	if (isTimeUp())
		return SVPair(-1, pos.getEval());
#endif

	int moves[MAX_MOVES];
	int totMoves = pos.generateMoves(moves);

	if (totMoves == 0) {
		Position newPos(pos);
		newPos.applyNullMove();
		Value val = -alphabeta(depth, -beta, -alpha, newPos).second;
		return SVPair(-1, val);
	}

	if (lastsv.first != -1) {
		for (int i = 0; i < totMoves; ++i) {
			if (moves[i] == lastsv.first) {
				std::swap(moves[0], moves[i]);
				break;
			}
		}
		assert(moves[0] == lastsv.first);
	}

	Value bestValue = -BND;
	int bestMove = -1;
	HashType hasht = ALPHA;

	for (int i = 0; i < totMoves; ++i) {
		Position newPos(pos);
		newPos.applyMove(moves[i]);

		Value val;
		val = -alphabeta(depth - 1, -beta, -alpha, newPos).second;

		if (val > bestValue) {
			bestValue = val;
			bestMove = moves[i];
			if (val >= beta) {
				recordHash(pos, bestValue, depth, BETA, bestMove);
				return SVPair(bestMove, bestValue);
			}
			if (val > alpha) {
				alpha = val;
				hasht = EXACT;
			}
		}
	}
	recordHash(pos, bestValue, depth, hasht, bestMove);

	return SVPair(bestMove, bestValue);
}

SVPair getBestMove(const Position & pos, int & maxdepth)
{
	initHashTable();
	startTime = clock();
	SVPair ret(-1, 0);
	for (int depth = 1; depth <= 64; ++depth) {
		SVPair cur = alphabeta(depth, -BND, +BND, pos, true);
		ret = cur;
		maxdepth = depth;
		//printf("Alpha-Beta2 > depth = %d, action = %d, value = %d\n", depth, ret.first, ret.second);
		if (isTimeUp())
			break;
	}
	//printf("Alpha-Beta2 > max depth = %d, value = %d, time cost is %.2f\n", maxdepth, ret.second,
	//	1.0 * (clock() - startTime) / CLOCKS_PER_SEC);
	return ret;
}

void printBestPath(int depth, const Position &pos)
{
	Value val = pos.isGameEnd() ? pos.getGameEndEval() : pos.getEval();
	printf("dep=%d, val=%d\n", depth, val);
	pos.print();

	if (depth != 0) {
		bool cut = false;
		SVPair lastsv = probeHash(depth, -BND, BND, pos, cut);
		assert(cut);
		Position newPos(pos);
		newPos.applyMove(lastsv.first);
		printBestPath(depth - 1, newPos);
	}
}

