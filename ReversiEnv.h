#pragma once
#include "bitboard.h"

#include <cassert>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <cstdio>
#include <cmath>

#define EMPTY(x, i, j) (!((x) & (1ULL << (((i) << 3) | (j)))))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SQUARE(x, y) (((x) << 3) | (y))

enum Player
{
	White = 0, Black = 1
};

enum Winner
{
	white = 0, black = 1, draw = 2, empty = -1
};

class Board
{
private:
	uint64 black, white;
public:
	Board()
	{
		this->black = (0x0000000000000008ULL << 32) | (0x0000000000000010ULL << 24);
		this->white = (0x0000000000000008ULL << 24) | (0x0000000000000010ULL << 32);
	}

	Board(uint64 black, uint64 white)
	{
		this->black = black, this->white = white;
	}

	uint64 get_white()
	{
		return white;
	}

	uint64 get_black()
	{
		return black;
	}

	int number_of_white()
	{
		return bit_count(white);
	}

	int number_of_black()
	{
		return bit_count(black);
	}
};


class ReversiEnv
{
public:
	Board board;
	Player curr_player;
	bool done;
	Winner winner;
	int turn;
	bool pass_auto;

private:
	int gg;
	int basicValue;
	uint64 hashCode;

public:
	ReversiEnv();
	ReversiEnv(bool pass_auto);
	~ReversiEnv();
	void reset();
	void set(uint64 black, uint64 white, Player curr_player, bool done = false);
	void step(int action);
	void null_step();
	void random_step();
	Player get_next_player();
	void render(bool paint = true);
	uint64 get_own();
	uint64 get_enemy();
	void switch_to_next_player();
	int getValue(bool show = false);
	int endVaule();
	uint64 getHashValue();

private:
	void game_over();
	void let_another_player_win();
};

