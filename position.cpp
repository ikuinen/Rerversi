#ifndef _BOTZONE_ONLINE
#include "position.h"
#include <cassert>
#endif

Position::Position()
{
	board[0] = (1ull << toSquare(3, 4)) | (1ull << toSquare(4, 3));
	board[1] = (1ull << toSquare(3, 3)) | (1ull << toSquare(4, 4));
	player = 0;
	nullMoveCount = 0;
	basicEval = 0;
	hashValue = PIECE_HASH_VALUE[toSquare(3, 3)][1]
		^ PIECE_HASH_VALUE[toSquare(3, 4)][0]
		^ PIECE_HASH_VALUE[toSquare(4, 3)][0]
		^ PIECE_HASH_VALUE[toSquare(4, 4)][1];
}

int Position::generateMoves(int moves[]) const
{
	u64 movesBB = getMovesBB(board[player], board[player ^ 1]);
	int tot = 0;
	while (movesBB) {
		moves[tot++] = getlsbid(movesBB);
		movesBB ^= getlsb(movesBB);
	}
	assert(tot <= MAX_MOVES);
	return tot;
}

void Position::applyMove(int sq)
{
	place(sq);
	getFlipped(sq, player, board[player], board[player ^ 1], basicEval, hashValue);
	nullMoveCount = 0;
	changePlayer();
}

void Position::applyMoveWithHint(int sq, int hint)
{
}

void Position::applyNullMove()
{
	assert(nullMoveCount <= 1);

	++nullMoveCount;
	changePlayer();
}

Value Position::getEval(bool show) const
{
	int my_tiles = 0, opp_tiles = 0, my_front_tiles = 0, opp_front_tiles = 0;
	double p = 0, c = 0, l = 0, m = 0, f = 0, d = 0;

	d = basicEval;

	// Piece difference, frontier disks and disk squares
	u64 frontier = getFrontierBB(board[player], board[player ^ 1]);
	my_tiles = countMyPieces();
	opp_tiles = countOppPieces();
	my_front_tiles = popcount(frontier & board[player]);
	opp_front_tiles = popcount(frontier & board[player ^ 1]);
	if (show)
		printf("frontier: %llu %d %d %llu %d %d\n", frontier, my_front_tiles, opp_front_tiles,
			frontier & board[player], popcount(frontier & board[player]), popcount(frontier & board[player ^ 1]));
	if (my_tiles > opp_tiles)
		p = (100.0 * my_tiles) / (my_tiles + opp_tiles);
	else if (my_tiles < opp_tiles)
		p = -(100.0 * opp_tiles) / (my_tiles + opp_tiles);
	if (my_front_tiles > opp_front_tiles)
		f = -(100.0 * my_front_tiles) / (my_front_tiles + opp_front_tiles);
	else if (my_front_tiles < opp_front_tiles)
		f = (100.0 * opp_front_tiles) / (my_front_tiles + opp_front_tiles);

	// Corner occupancy
	my_tiles = opp_tiles = 0;
	my_tiles += isMyPiece(toSquare(0, 0));
	opp_tiles += isOppPiece(toSquare(0, 0));
	my_tiles += isMyPiece(toSquare(0, 7));
	opp_tiles += isOppPiece(toSquare(0, 7));
	my_tiles += isMyPiece(toSquare(7, 0));
	opp_tiles += isOppPiece(toSquare(7, 0));
	my_tiles += isMyPiece(toSquare(7, 7));
	opp_tiles += isOppPiece(toSquare(7, 7));
	c = 25 * (my_tiles - opp_tiles);

	// Corner closeness
	my_tiles = opp_tiles = 0;
	if (isEmpty(toSquare(0, 0))) {
		my_tiles += isMyPiece(toSquare(0, 1));
		opp_tiles += isOppPiece(toSquare(0, 1));
		my_tiles += isMyPiece(toSquare(1, 0));
		opp_tiles += isOppPiece(toSquare(1, 0));
		my_tiles += isMyPiece(toSquare(1, 1));
		opp_tiles += isOppPiece(toSquare(1, 1));
	}
	if (isEmpty(toSquare(0, 7))) {
		my_tiles += isMyPiece(toSquare(0, 6));
		opp_tiles += isOppPiece(toSquare(0, 6));
		my_tiles += isMyPiece(toSquare(1, 6));
		opp_tiles += isOppPiece(toSquare(1, 6));
		my_tiles += isMyPiece(toSquare(1, 7));
		opp_tiles += isOppPiece(toSquare(1, 7));
	}
	if (isEmpty(toSquare(7, 0))) {
		my_tiles += isMyPiece(toSquare(6, 0));
		opp_tiles += isOppPiece(toSquare(6, 0));
		my_tiles += isMyPiece(toSquare(6, 1));
		opp_tiles += isOppPiece(toSquare(6, 1));
		my_tiles += isMyPiece(toSquare(7, 1));
		opp_tiles += isOppPiece(toSquare(7, 1));
	}
	if (isEmpty(toSquare(7, 7))) {
		my_tiles += isMyPiece(toSquare(6, 6));
		opp_tiles += isOppPiece(toSquare(6, 6));
		my_tiles += isMyPiece(toSquare(6, 7));
		opp_tiles += isOppPiece(toSquare(6, 7));
		my_tiles += isMyPiece(toSquare(7, 6));
		opp_tiles += isOppPiece(toSquare(7, 6));
	}
	l = -12.5 * (my_tiles - opp_tiles);

	// Mobility
	my_tiles = popcount(getMovesBB(board[player], board[player ^ 1]));
	opp_tiles = popcount(getMovesBB(board[player ^ 1], board[player]));
	if (my_tiles > opp_tiles)
		m = (100.0 * my_tiles) / (my_tiles + opp_tiles);
	else if (my_tiles < opp_tiles)
		m = -(100.0 * opp_tiles) / (my_tiles + opp_tiles);

	if (show) {
		printf("10 * p = %f\n", 10 * p);
		printf("801.724 * c = %f\n", 801.724 * c);
		printf("382.026 * l = %f\n", 382.026 * l);
		printf("78.922 * m = %f\n", 78.922 * m);
		printf("74.396 * f = %f\n", 74.396 * f);
		printf("10 * d = %f\n", 10 * d);
	}

	// final weighted score
	double score = ((10 * p) + (801.724 * c) + (382.026 * l) + (78.922 * m) + (74.396 * f) + (10 * d)) * 100;

	return (Value)score;
}

void Position::print() const
{
	//printf("Round #%d\n", countAllPieces() - 4 + 1);
	printf("white = %d, black = %d\n", countWhitePieces(), countBlackPieces());
	return;
	if (isBlackPlayer())
		printf("Now BLACK\n");
	else
		printf("Now WHITE\n");
	printf("Hash Value: %lld\n", getHashValue());
	printf("Evaluation: %d\n", getEval());

	printf("  ");
	for (int i = 0; i < BOARD_WIDTH; ++i) {
		printf("%c ", 'a' + i);
	}
	printf("\n");
	for (int i = 0; i < BOARD_WIDTH; ++i) {
		printf("%d ", i + 1);
		for (int j = 0; j < BOARD_WIDTH; ++j) {
			int sq = toSquare(i, j);
			if (isEmpty(sq))
				printf(". ");
			else if (isBlackPiece(sq))
				printf("b ");
			else {
				assert(isWhitePiece(sq));
				printf("w ");
			}
		}
		printf("\n");
	}
}