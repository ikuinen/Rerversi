#pragma once
#include "ReversiEnv.h"
#include "bitboard.h"
#include <cmath>
#include <stack>
#include <cstring>

#define NUMBER_NODE 89991
#define BUFFER_SIZE 9221

#define ALPHA_BETA
#define REPLACE_TABLE

#ifdef ALPHA_BETA

#define HASH_TABLE_SIZE  1 << 23
#define HASH_TABLE_MASK  (HASH_TABLE_SIZE - 1)


enum HashType {
	EXACT,
	ALPHA,
	BETA
};

struct HashItem {
	uint64 key;
	int value;
	int depth;
	HashType type;
	int bestMove;
};

struct Pair
{
	int action;
	int value;

	Pair(int action, int value) :action(action), value(value)
	{

	}
};

#endif // ALPHA_BETA


struct MCTSNode
{
	int N;
	double Q;
	uint64 key;
	int last_move;
	uint64 all_moves;
	int parent, firstChild, nextSibling;

	void reset() {
		parent = firstChild = nextSibling = -1;
		key = all_moves = 0ULL;
		N = 0; Q = 0;
	}
};

struct MCTSBuffer
{
	uint64 key;
	int N;
	double Q;
};

struct Config
{
	int search_times;
	int simulation_times;
	int max_probe_depth;
	double c_explore, c_local;
	int max_estimate_depth;
	double time_limitation;
	Config()
	{
		search_times = 15000;
		simulation_times = 1;
		max_probe_depth = 63;
		c_explore = 0.30;
		max_estimate_depth = 7;
		c_local = 0.68; // 0.56 -> 0.62 -> 0.68
		time_limitation = 2.2;
	}
};

struct Simluation
{
	double Q;
	int N;

	Simluation(double Q, int N) :Q(Q), N(N) {  }

};

class JPlayer
{
private:
	MCTSNode nodes[NUMBER_NODE];
	MCTSBuffer buffer[BUFFER_SIZE];
	int rIndex;
	Config config;
	int available_ptr;
	clock_t startTime;

	void search(ReversiEnv &env, int rIndex);
	int expand(ReversiEnv &env, int root);
	int UCTSelect(ReversiEnv &env, int root);
	int JUCTSelect(ReversiEnv &env, int root);
	Simluation simulate(ReversiEnv &env, int root);
	int allocate();
	int cool_action2(ReversiEnv &env);

#ifdef ALPHA_BETA
	Pair probe(ReversiEnv &env, int depth, int alpha, int beta, bool &cut);
	void record(ReversiEnv &env, int depth, HashType type, Pair pair);
	Pair alpha_beta(ReversiEnv &env, int depth, int alpha, int beta);
	int cool_action1(ReversiEnv &env);
#endif

public:
	JPlayer();
	~JPlayer();
	void clear();
	int action(ReversiEnv &env, int choice = 1);
	int estimate_value(ReversiEnv &env, int low, int upper, int depth);
};

