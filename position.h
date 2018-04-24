#ifndef POSITION_H_
#define POSITION_H_

#ifndef _BOTZONE_ONLINE
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <string>
#include "constant.h"
#include "bitboard2.h"
#endif

inline int toSquare(int x, int y)
{
	return x * BOARD_WIDTH + y;
}

inline int getSquareX(int sq)
{
	return sq / BOARD_WIDTH;
}

inline int getSquareY(int sq)
{
	return sq % BOARD_WIDTH;
}

class Position {
public:
	Position();

	bool isBlackPlayer() const;
	bool isWhitePlayer() const;
	bool isEmpty(int sq) const;
	bool isMyPiece(int sq) const;
	bool isOppPiece(int sq) const;
	bool isBlackPiece(int sq) const;
	bool isWhitePiece(int sq) const;
	int countAllPieces() const;
	int countBlackPieces() const;
	int countWhitePieces() const;
	int countMyPieces() const;
	int countOppPieces() const;
	bool isGameEnd() const;
	int generateMoves(int moves[]) const;

	Value getEval(bool show = false) const;
	Value getGameEndEval() const;

	HashValue getHashValue() const;

	void print() const;

	void changePlayer();
	void applyMove(int sq);
	void applyMoveWithHint(int sq, int hint);
	void applyNullMove();

private:
	u64 board[2];
	int player;
	int nullMoveCount;
	int basicEval;
	HashValue hashValue;

	void place(int sq);
	void replace(int sq);
};

inline bool Position::isBlackPlayer() const
{
	return player == 0;
}

inline bool Position::isWhitePlayer() const
{
	return player == 1;
}

inline bool Position::isEmpty(int sq) const
{
	return !(board[0] >> sq & 1) && !(board[1] >> sq & 1);
}

inline bool Position::isMyPiece(int sq) const
{
	return board[player] >> sq & 1;
}

inline bool Position::isOppPiece(int sq) const
{
	return board[player ^ 1] >> sq & 1;
}

inline bool Position::isBlackPiece(int sq) const
{
	return board[0] >> sq & 1;
}

inline bool Position::isWhitePiece(int sq) const
{
	return board[1] >> sq & 1;
}

inline int Position::countAllPieces() const
{
	return popcount(board[0] | board[1]);
}

inline int Position::countBlackPieces() const
{
	return popcount(board[0]);
}

inline int Position::countWhitePieces() const
{
	return popcount(board[1]);
}

inline int Position::countMyPieces() const
{
	return popcount(board[player]);
}

inline int Position::countOppPieces() const
{
	return popcount(board[player ^ 1]);
}

inline bool Position::isGameEnd() const
{
	return countAllPieces() == BOARD_SIZE || nullMoveCount > 1;
}

inline Value Position::getGameEndEval() const
{
	int d = countMyPieces() - countOppPieces();
	if (d == 0)
		return 0;
	return (d > 0 ? +INF : -INF) + d;
}

inline HashValue Position::getHashValue() const
{
	return hashValue << 1 | player;
}

inline void Position::changePlayer()
{
	player ^= 1;
	basicEval = -basicEval;
}

inline void Position::place(int sq)
{
	assert(isEmpty(sq));

	board[player] ^= 1ull << sq;
	basicEval += VALUE[sq];
	hashValue ^= PIECE_HASH_VALUE[sq][player];
}

inline void Position::replace(int sq)
{
}

#endif