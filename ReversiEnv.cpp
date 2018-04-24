#include "ReversiEnv.h"


ReversiEnv::ReversiEnv():ReversiEnv(true)
{
}

ReversiEnv::ReversiEnv(bool pass_auto)
{
	this->pass_auto = pass_auto;
	srand(time(0));
	reset();
}


ReversiEnv::~ReversiEnv()
{
}

void ReversiEnv::reset()
{
	board = Board();
	winner = empty;
	curr_player = Black;
	done = false;
	gg = 0;
	turn = 0;
	hashCode = 0ULL;
	basicValue = 0;

	hashCode = HASH_VALUE[SQUARE(3, 3)][1]
		^ HASH_VALUE[SQUARE(3, 4)][0]
		^ HASH_VALUE[SQUARE(4, 3)][0]
		^ HASH_VALUE[SQUARE(4, 4)][1];
}

void ReversiEnv::set(uint64 black, uint64 white, Player curr_player, bool done)
{
	board = Board(black, white);
	this->curr_player = curr_player;
	winner = empty;
	this->done = done;
	turn = board.number_of_black() + board.number_of_white() - 4;
	gg = 0;
	hashCode = 0ULL;
	basicValue = 0;

	for(int i = 0; i < 8; i++)
		for (int j = 0; j < 8; j++) {
			if (!EMPTY(black, i, j)) {
				if (curr_player == Black)
					basicValue += V[i][j];
				else
					basicValue -= V[i][j];
				hashCode ^= HASH_VALUE[SQUARE(i, j)][0];
			}
			else if (!EMPTY(white, i, j)) {
				if (curr_player == White)
					basicValue += V[i][j];
				else
					basicValue -= V[i][j];
				hashCode ^= HASH_VALUE[SQUARE(i, j)][1];
			}
		}
}

void ReversiEnv::step(int action)
{
	assert((action >= 0 && action <= 63) || action == -1);

	if (done)
		return;

	if (action == -1) {
		printf("give up\n");
		system("pause");
		let_another_player_win();
		game_over();
		return;
	}

	uint64 own, enemy, tmp;
	if (this->curr_player == Black)
		own = board.get_black(), enemy = board.get_white();
	else
		enemy = board.get_black(), own = board.get_white();

	tmp = own;
	do_flip(action, own, enemy, basicValue, hashCode, (curr_player == White) ? 1 : 0);
	if (tmp == own) {
		printf("illegal action.\n, %d: (%d, %d)", action, action / 8 + 1, action % 8 + 1);
		this->render();
		own |= (1ULL << action);
		this->render();
		system("pause");
		exit(0);
		let_another_player_win();
		game_over();
		return;
	}

	own |= (1ULL << action);
	turn += 1;
	gg = 0;
	

	if (this->curr_player == Black)
		board = Board(own, enemy);
	else
		board = Board(enemy, own);

	basicValue += V[action >> 3][action & 0x7];
	hashCode ^= HASH_VALUE[action][(curr_player == White) ? 1 : 0];

	int e = bit_count(find_correct_moves(enemy, own)), 
		m = bit_count(find_correct_moves(own, enemy));

	if (e == 0 && m == 0) // e == 0, m == 0
		game_over();
	if (e > 0 || !pass_auto) //e > 0 or (e == 0 && pass_auto)
		switch_to_next_player();
}

void ReversiEnv::null_step()
{
	switch_to_next_player();
	gg += 1;
	if (gg >= 2)
		game_over();
}

Player ReversiEnv::get_next_player()
{
	if (this->curr_player == White)
		return Black;
	else
		return White;
}

uint64 ReversiEnv::get_own()
{
	if (this->curr_player == Black)
		return board.get_black();
	else
		return board.get_white();
}

uint64 ReversiEnv::get_enemy()
{
	if (this->curr_player == White)
		return board.get_black();
	else
		return board.get_white();
}

void ReversiEnv::switch_to_next_player()
{
	this->curr_player = get_next_player();
	basicValue = -basicValue;
}

int ReversiEnv::getValue(bool show)
{
	uint64 me = get_own(), enemy = get_enemy();
	double p = 0, c = 0, l = 0, m = 0, f = 0, d = 0;
	int my_tiles = 0, opp_tiles = 0, i, j, k, my_front_tiles = 0, opp_front_tiles = 0, x, y;
	const int dx[] = { -1, -1, 0, 1, 1, 1, 0, -1 };
	const int dy[] = { 0, 1, 1, 1, 0, -1, -1, -1 };

	my_tiles = bit_count(me);
	opp_tiles = bit_count(enemy);

	d = basicValue;

	uint64 frontier = getFrontier(me, enemy);

	my_front_tiles = bit_count(frontier & me);
	opp_front_tiles = bit_count(frontier & enemy);

	if (show)
		printf("frontier: %llu %d %d %llu %d\n", frontier, my_front_tiles, opp_front_tiles, frontier & me, bit_count(frontier & me));

	if (my_tiles > opp_tiles)
		p = (100.0 * my_tiles) / (my_tiles + opp_tiles);
	else if (my_tiles < opp_tiles)
		p = -(100.0 * opp_tiles) / (my_tiles + opp_tiles);
	else
		p = 0;

	if (my_front_tiles > opp_front_tiles)
		f = -(100.0 * my_front_tiles) / (my_front_tiles + opp_front_tiles);
	else if (my_front_tiles < opp_front_tiles)
		f = (100.0 * opp_front_tiles) / (my_front_tiles + opp_front_tiles);
	else
		f = 0;

	//2.Corner occupancy
	my_tiles = opp_tiles = 0;
	if (!EMPTY(me, 0, 0)) my_tiles++;
	else if (!EMPTY(enemy, 0, 0)) opp_tiles++;
	if (!EMPTY(me, 0, 7)) my_tiles++;
	else if (!EMPTY(enemy, 0, 7)) opp_tiles++;
	if (!EMPTY(me, 7, 0)) my_tiles++;
	else if (!EMPTY(enemy, 7, 0)) opp_tiles++;
	if (!EMPTY(me, 7, 7)) my_tiles++;
	else if (!EMPTY(enemy, 7, 7)) opp_tiles++;
	c = 25 * (my_tiles - opp_tiles);

	//3. Corner closeness
	my_tiles = opp_tiles = 0;
	if (EMPTY(me, 0, 0) && EMPTY(enemy, 0, 0)) {
		if (!EMPTY(me, 0, 1)) my_tiles++;
		else if (!EMPTY(enemy, 0, 1)) opp_tiles++;
		if (!EMPTY(me, 1, 0)) my_tiles++;
		else if (!EMPTY(enemy, 1, 0)) opp_tiles++;
		if (!EMPTY(me, 1, 1)) my_tiles++;
		else if (!EMPTY(enemy, 1, 1)) opp_tiles++;
	}
	if (EMPTY(me, 0, 7) && EMPTY(enemy, 0, 7)) {
		if (!EMPTY(me, 0, 6)) my_tiles++;
		else if (!EMPTY(enemy, 0, 6)) opp_tiles++;
		if (!EMPTY(me, 1, 7)) my_tiles++;
		else if (!EMPTY(enemy, 1, 7)) opp_tiles++;
		if (!EMPTY(me, 1, 6)) my_tiles++;
		else if (!EMPTY(enemy, 1, 6)) opp_tiles++;
	}
	if (EMPTY(me, 7, 0) && EMPTY(enemy, 7, 0)) {
		if (!EMPTY(me, 6, 1)) my_tiles++;
		else if (!EMPTY(enemy, 6, 1)) opp_tiles++;
		if (!EMPTY(me, 7, 1)) my_tiles++;
		else if (!EMPTY(enemy, 7, 1)) opp_tiles++;
		if (!EMPTY(me, 6, 0)) my_tiles++;
		else if (!EMPTY(enemy, 6, 0)) opp_tiles++;
	}
	if (EMPTY(me, 7, 7) && EMPTY(enemy, 7, 7)) {
		if (!EMPTY(me, 6, 6)) my_tiles++;
		else if (!EMPTY(enemy, 6, 6)) opp_tiles++;
		if (!EMPTY(me, 7, 6)) my_tiles++;
		else if (!EMPTY(enemy, 7, 6)) opp_tiles++;
		if (!EMPTY(me, 6, 7)) my_tiles++;
		else if (!EMPTY(enemy, 6, 7)) opp_tiles++;
	}
	l = -12.5 * (my_tiles - opp_tiles);

	//4. Mobility
	my_tiles = bit_count(find_correct_moves(me, enemy));
	opp_tiles = bit_count(find_correct_moves(enemy, me));
	if (my_tiles > opp_tiles)
		m = (100.0 * my_tiles) / (my_tiles + opp_tiles);
	else if (my_tiles < opp_tiles)
		m = -(100.0 * opp_tiles) / (my_tiles + opp_tiles);
	else m = 0;

	if (show) {
		printf("10 * p = %f\n", 10 * p);
		printf("801.724 * c = %f\n", 801.724 * c);
		printf("382.026 * l = %f\n", 382.026 * l);
		printf("78.922 * m = %f\n", 78.922 * m);
		printf("74.396 * f = %f\n", 74.396 * f);
		printf("10 * d = %f\n", 10 * d);
	}

	double score = (10 * p) + (801.724 * c) + (382.026 * l) + (78.922 * m) + (74.396 * f) + (10 * d);
	return score * 100;
}

int ReversiEnv::endVaule()
{
	int diff = bit_count(get_own()) - bit_count(get_enemy());
	if (diff == 0)
		return 0.0;
	if (diff > 0)
		return 100 + diff;
	else
		return -100 + diff;
}

uint64 ReversiEnv::getHashValue()
{
	return hashCode << 1 | (curr_player == White ? 1 : 0);
}

void ReversiEnv::game_over()
{
	done = true;
	if (winner == empty) {
		int black_num = bit_count(board.get_black()), white_num = bit_count(board.get_white());
		if (black_num > white_num)
			winner = black;
		else if (black_num < white_num)
			winner = white;
		else
			winner = draw;
	}
}

void ReversiEnv::let_another_player_win()
{
	if (this->curr_player == White)
		this->winner = black;
	else
		this->winner = white;
}

void ReversiEnv::random_step()
{
	int legal_moves[32], length;
	if (this->curr_player == Black) {
		length = bit_to_array(find_correct_moves(board.get_black(), board.get_white()), legal_moves);
	}
	else {
		length = bit_to_array(find_correct_moves(board.get_white(), board.get_black()), legal_moves);
	}
	if (length == 0) {
		null_step();
		return;
	}
	int index = rand() % length;
	this->step(legal_moves[index]);
}

void ReversiEnv::render(bool paint)
{
	uint64 black = board.get_black(), white = board.get_white();
	int blackCount = board.number_of_black(), whiteCount = board.number_of_white();
	printf("turn %d> white = %d, black = %d, curr_player = %s\n", turn, 
		whiteCount, blackCount, curr_player == White ? "White" : "Black");
	if (!paint)
		return;
	printf("#");
	for (int i = 1; i <= 8; i++)
		printf("\t%d", i);
	printf("\t#\n\n");
	for (int i = 1; i <= 8; i++) {
		printf("%d", i);
		for (int j = 1; j <= 8; j++) {
			if (white & 1)
				printf("\t%s", "¡ð");
			else if (black & 1)
				printf("\t%s", "¡ñ");
			else
				printf("\t ");
			white >>= 1, black >>= 1;
		}
		printf("\t#\n\n");
	}
}