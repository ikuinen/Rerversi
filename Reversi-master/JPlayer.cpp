#include "JPlayer.h"

int q2[32], cnt, conflict;

#ifdef ALPHA_BETA
#ifdef REPLACE_TABLE
HashItem replaceTable[HASH_TABLE_SIZE + 1];
#endif
#endif 

JPlayer::JPlayer()
{
	clear();
}


JPlayer::~JPlayer()
{
}

void JPlayer::clear()
{
	memset(nodes, 0, sizeof(nodes));
	memset(buffer, 0, sizeof(buffer));
	rIndex = -1;
}

int JPlayer::action(ReversiEnv & env, int choice)
{
	ReversiEnv myEnv = env;
	myEnv.pass_auto = false;
#ifdef ALPHA_BETA
	if (choice == 1) // Alpha-Beta
		return this->cool_action1(myEnv);
#endif
	if (choice == 2) // MCTS
		return this->cool_action2(myEnv);
	else if (choice == 3) { //MCTS + Alpha-Beta
		if (env.turn <= 24)
			return this->cool_action2(myEnv);
		else
			return this->cool_action1(myEnv);
	}
	else if (choice == 4) { //Alpha-Beta + MCTS
		if (env.turn <= 30)
			return this->cool_action1(myEnv);
		else
			return this->cool_action2(myEnv);
	}
	else
		return -1;
}

void JPlayer::search(ReversiEnv &env, int rIndex)
{
	MCTSNode &rNode = nodes[rIndex];
	ReversiEnv cEnv = env;
	int cIndex = rIndex;
	//!env.done && (nodes[cIndex].all_moves || nodes[cIndex].firstChild == -1)
	while (!cEnv.done) { //select
		if (nodes[cIndex].all_moves || nodes[cIndex].firstChild == -1)
			break;
		cIndex = UCTSelect(cEnv, cIndex);
		if (nodes[cIndex].last_move != -1)
			cEnv.step(nodes[cIndex].last_move);
		else
			cEnv.null_step();

		int id = cEnv.getHashValue() % BUFFER_SIZE;
		buffer[id].N = nodes[cIndex].N;
		buffer[id].Q = nodes[cIndex].Q;
		buffer[id].key = nodes[cIndex].key;
	}

	if (!cEnv.done) {
		cIndex = expand(cEnv, cIndex);
		if (nodes[cIndex].last_move != -1)
			cEnv.step(nodes[cIndex].last_move);
		else
			cEnv.null_step();
		nodes[cIndex].key = cEnv.getHashValue();
		nodes[cIndex].all_moves = find_correct_moves(cEnv.get_own(), cEnv.get_enemy());
	}
		

	Simluation result = simulate(cEnv, cIndex); //simulate
	MCTSNode *ptr = &nodes[cIndex];
	//result.Q = -result.Q;
	//back-progagate
	while (ptr->key != rNode.key) {
		ptr->N += result.N;
		ptr->Q += result.Q;
		result.Q = -result.Q;
		ptr = &nodes[ptr->parent];
	}
	ptr->N += result.N;
	ptr->Q += result.Q;
}

int JPlayer::expand(ReversiEnv & env, int root)
{
	assert(!env.done);
	assert(nodes[root].all_moves || nodes[root].firstChild == -1);
	int child[32], len;
	len = bit_to_array(nodes[root].all_moves, child);
	int index = allocate();
	nodes[index].parent = root;
	nodes[index].nextSibling = nodes[root].firstChild;
	nodes[root].firstChild = index;
	if (len == 0) {
		nodes[index].last_move = -1;
	}
	else {
		int action_index = rand() % len;
		nodes[index].last_move = child[action_index];
		nodes[root].all_moves ^= (1ULL << child[action_index]);
	}
	return index;
}

int JPlayer::UCTSelect(ReversiEnv &env, int root)
{
	assert(!env.done);
	assert(!nodes[root].all_moves && nodes[root].firstChild != -1);
	int max_index = -1;
	double max_ucb1;
	for (int cIndex = nodes[root].firstChild; cIndex != -1; cIndex = nodes[cIndex].nextSibling) {
		double score1 = -1.0 * nodes[cIndex].Q / 100.0 / nodes[cIndex].N,
			score2 = config.c_explore * sqrt(2.0 * log2(nodes[root].N / nodes[cIndex].N));
		double ucb1 = score1 + score2;
		if (max_index == -1 || ucb1 > max_ucb1) {
			max_index = cIndex;
			max_ucb1 = ucb1;
		}
	}
	assert(max_index != -1);
	return max_index;
}

int JPlayer::JUCTSelect(ReversiEnv & env, int root)
{
	assert(!env.done);
	assert(!nodes[root].all_moves && nodes[root].firstChild != -1);
	int max_index = -1, len = 0, sum = 0;
	for (int cIndex = nodes[root].firstChild; cIndex != -1; cIndex = nodes[cIndex].nextSibling) {
		ReversiEnv nextEnv = env;
		if (nodes[cIndex].last_move == -1)
			nextEnv.null_step();
		else
			nextEnv.step(nodes[cIndex].last_move);
		q2[len] = -estimate_value(nextEnv, -999999999, 999999999, config.max_estimate_depth);
		sum += abs(q2[len++]);
	}
	/*for (int i = 0; i < len; i++)
		printf("%d ", q2[i]);
	printf("\n");*/
	if (sum <= 0) {
		for (int i = 0; i < len; i++)
			printf("%d\n", q2[i]);
		printf("%d\n", sum);
		system("pause");
	}
	int max_ptr;
	double max_ucb1;
	len = 0;
	for (int cIndex = nodes[root].firstChild; cIndex != -1; cIndex = nodes[cIndex].nextSibling) {
		double score1 = -1.0 * nodes[cIndex].Q / 100.0 / nodes[cIndex].N,
			score2 = config.c_explore * sqrt(2.0 * log2(nodes[root].N / nodes[cIndex].N)),
			score3 = config.c_local * q2[len++] / sum; // l1 normalize
		//printf("score1 = %.2f, score2 = %.2f, score3 = %.2f\n", score1, score2, score3);
		//printf("score3 / score1 = %.2f\n", score3 / score1);
		double ucb1 = score1 + score2 + score3;
		if (max_index == -1 || ucb1 > max_ucb1) {
			max_ptr = len - 1;
			max_index = cIndex;
			max_ucb1 = ucb1;
		}
	}
	//printf("choose local value = %d\n", q2[max_ptr]);
	assert(max_index != -1);
	return max_index;
}

int JPlayer::estimate_value(ReversiEnv &env, int low, int upper, int depth)
{
#ifdef _DEBUG
	//env.render();
#endif 

	int len, legal_moves[32];
	if (env.done)
		return env.endVaule() * 1000000;
	if (depth == 0)
		return env.getValue();

	len = bit_to_array(find_correct_moves(env.get_own(), env.get_enemy()), legal_moves);

	if (len == 0) {
		ReversiEnv next_env = env;
		next_env.null_step();
		return -estimate_value(next_env, -upper, -low, depth);
	}

	int i;
	double max_value;
	bool first = true;
	ReversiEnv next_env = env;

	next_env.step(legal_moves[0]);
	max_value = -next_env.getValue();
	for (i = 1; i < len; i++) {
		next_env = env;
		next_env.step(legal_moves[i]);
		double value = -next_env.getValue();
		if (value > max_value)
			max_value = value,
			std::swap(legal_moves[0], legal_moves[i]);
	}

	for (i = 0; i < len; i++) {
		next_env = env;
		next_env.step(legal_moves[i]);
		double value = -estimate_value(next_env, -upper, -low, depth - 1);
		if (first || value > max_value) {
			max_value = value;
			first = false;
			if (max_value >= upper)
				return max_value;
			if (max_value > low)
				low = max_value;
		}
	}
	//printf("%d %d\n", i, len);
	return max_value;
}

Simluation JPlayer::simulate(ReversiEnv & env, int root)
{
	double total_value = 0;
	int extra_simulation_times = 0;
	int id = env.getHashValue() % BUFFER_SIZE;
	if (buffer[id].key == env.getHashValue()) {
		//printf("hit in the buffer\n");
		cnt++;
		total_value = buffer[id].Q,
		extra_simulation_times = buffer[id].N;
	}
	for (int i = 0; i < config.simulation_times; i++) {
		ReversiEnv cEnv = env;
		while (!cEnv.done) {
			if (rand() & (0x3f))
				cEnv.random_step();
			else {
				int max_index = -1, max_value;
				int legal_moves[32], len;
				len = bit_to_array(find_correct_moves(cEnv.get_own(), cEnv.get_enemy()), legal_moves);
				if (len == 0)
					cEnv.null_step();
				else if (!(rand() & (0x3)))
					cEnv.random_step();
				else {
					for (int i = 0; i < len; i++) {
						ReversiEnv gEnv = cEnv;
						gEnv.step(legal_moves[i]);
						int value = -estimate_value(gEnv, -99999999, 99999999, rand() & (0x1));
						if ((max_index == -1 || max_value < value)) {
							max_index = i;
							max_value = value;
						}
					}
					cEnv.step(legal_moves[max_index]);
				}
			}
		}
		int value = cEnv.endVaule();
		if (cEnv.curr_player == env.curr_player)
			total_value += value;
		else
			total_value -= value;
	}
	return Simluation(1.0 * total_value / (config.simulation_times + extra_simulation_times), 1);
}

int JPlayer::allocate()
{
	nodes[available_ptr].reset();
	return available_ptr++;
}

int JPlayer::cool_action2(ReversiEnv & env)
{
	available_ptr = 0;
	cnt = 0;
	conflict = 0;
	config.c_explore = 0.5;
	int index = -1;
	memset(nodes, 0, sizeof(nodes));
	rIndex = allocate();
	nodes[rIndex].key = env.getHashValue();
	nodes[rIndex].all_moves = find_correct_moves(env.get_own(), env.get_enemy());
	assert(nodes[rIndex].all_moves > 0ULL);
	startTime = clock();
	for (int i = 0; i < config.search_times; i++) 
		search(env, rIndex);
	if (env.turn < 10)
		config.max_estimate_depth = 8;
	else if (env.turn < 20)
		config.max_estimate_depth = 8;
	else if (env.turn < 25)
		config.max_estimate_depth = 8;
	else if (env.turn < 30)
		config.max_estimate_depth = 8;
	else if (env.turn < 40)
		config.max_estimate_depth = 8;
	else if (env.turn < 45)
		config.max_estimate_depth = 12;
	else if (env.turn < 50)
		config.max_estimate_depth = 16;
	else if (env.turn < 60)
		config.max_estimate_depth = 63;
	//printf("%d %d\n", env.turn, config.max_estimate_depth);
	config.c_explore = 0.001;
	index = JUCTSelect(env, rIndex);
	assert(nodes[index].last_move != -1);
	printf("MCTS> hit %d, time cost is %.2f, action = %d, estimate depth = %d\n", cnt,
		1.0 * (clock() - startTime) / CLOCKS_PER_SEC, nodes[index].last_move, config.max_estimate_depth);
	return nodes[index].last_move;
}

#ifdef ALPHA_BETA

Pair JPlayer::probe(ReversiEnv & env, int depth, int alpha, int beta, bool & cut)
{
	uint64 key = env.getHashValue();
	int index = key & HASH_TABLE_MASK;
	HashItem &node = replaceTable[index];
	if (node.key != key)
		return Pair(-1, -999999999);
	if (node.depth >= depth) {
		cut = true;
		if (node.type == EXACT)
			return Pair(node.bestMove, node.value);
		else if (node.type == ALPHA) {
			if (node.value <= alpha)
				return Pair(node.bestMove, alpha);
		}
		else {
			if (node.value >= beta)
				return Pair(node.bestMove, beta);
		}
		cut = false;
	}
	return Pair(node.bestMove, node.value);
}

void JPlayer::record(ReversiEnv & env, int depth, HashType type, Pair pair)
{
	uint64 key = env.getHashValue();
	int index = key & HASH_TABLE_MASK;
	HashItem &node = replaceTable[index];
	if (node.depth <= depth) {
		node.key = key;
		node.value = pair.value;
		node.bestMove = pair.action;
		node.type = type;
		node.depth = depth;
	}
}

Pair JPlayer::alpha_beta(ReversiEnv & env, int depth, int alpha, int beta)
{
	int legal_moves[32], len;
	int best_move = -1;
#ifdef REPLACE_TABLE
	if (env.done) {
		return Pair(-1, env.endVaule() * 10000000);
	}

	bool cut = false;
	Pair prev = probe(env, depth, alpha, beta, cut);
	if (cut && prev.action != -1)
		return prev;

	if (depth == 0) {
		int value = env.getValue();
		record(env, depth, EXACT, Pair(-1, value));
		return Pair(-1, value);
	}

	/*if ((clock() - startTime) >= config.time_limitation * CLOCKS_PER_SEC)
		return Pair(-1, env.getValue());*/

	len = bit_to_array(find_correct_moves(env.get_own(), env.get_enemy()), legal_moves);

	if (len == 0) {
		ReversiEnv nextEnv = env;
		nextEnv.null_step();
		int value = -alpha_beta(nextEnv, depth, -beta, -alpha).value;
		return Pair(-1, value);
	}

	if (prev.action != -1) {
		for (int i = 0; i < len; ++i) {
			if (legal_moves[i] == prev.action) {
				std::swap(legal_moves[0], legal_moves[i]);
				break;
			}
		}
	}

	int best_value = -999999999;
	HashType type = ALPHA;

	for (int i = 0; i < len; i++) {
		ReversiEnv nextEnv = env;
		nextEnv.step(legal_moves[i]);
		int value = -alpha_beta(nextEnv, depth - 1, -beta, -alpha).value;

		if (best_move == -1 || value > best_value) {
			best_value = value;
			best_move = legal_moves[i];
			if (value >= beta) {
				record(env, depth, BETA, Pair(best_move, best_value));
				return Pair(best_move, best_value);
			}
			if (value > alpha) {
				alpha = value;
				type = EXACT;
			}
		}
	}
	record(env, depth, type, Pair(best_move, best_value));
	return Pair(best_move, best_value);
#endif
}

int JPlayer::cool_action1(ReversiEnv & env)
{
	Pair result(-1, -1);
	int i, depth;
	memset(replaceTable, 0, sizeof(replaceTable));
	startTime = clock();
	config.max_probe_depth = 63;
	for (i = 1; i <= config.max_probe_depth; i++) {
		result = alpha_beta(env, i, -999999999, 999999999);
		//printf("Alpha-Beta > depth = %d, action = %d, value = %d\n", i, result.action, result.value);
		depth = i;
		if ((clock() - startTime) >= config.time_limitation * CLOCKS_PER_SEC)
			break;
	}
	printf("Alpha-Beta> max depth = %d, value = %d, time cost is %.2f\n", depth, result.value,
		1.0 * (clock() - startTime) / CLOCKS_PER_SEC);
	return result.action;
}

#endif