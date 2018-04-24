#include "ReversiEnv.h"
#include "JPlayer.h"
#include "bitboard.h"
#include "position.h"
#include "search.h"
#include <iostream>
#include <ctime>
#include <cstdlib>

using namespace std;
ReversiEnv env;
Position pos;
JPlayer computer1;

int main(int argc, char **agrv)
{
	int i, j;
	{ //test1
		int win = 0, total = 100;
		for (int i = 1; i <= total; i++) {
			clock_t start = clock();
			Player player;
			player = rand() % 2 ? Black : White;
			if (player == Black)
				printf("computer1 is black.\n");
			else
				printf("computer1 is white.\n");
			while (!env.done) {
				//env.render(false);
				env.render();
				if (env.curr_player == player) {
					int action1, depth = 0, action2;
					//printf("computer1: ");
					action1 = computer1.action(env, 1);
					env.step(action1);
					pos.applyMove(action1);
					if (env.curr_player == player)
						pos.applyNullMove();
				}
				else {
					int x, y;
					int action1, depth = 0, action2;
					printf("input the coordinates:\n");
					cin >> x >> y;
					x--, y--;
					action1 = (x << 3) + y;
					env.step(action1);
					pos.applyMove(action1);
					if (env.curr_player != player)
						pos.applyNullMove();
				}
			}
			if (env.winner == black && player == Black
				|| env.winner == white && player == White) {
				win++;
				printf("epoch %d: computer1 wins\n", i);
			}
			else
				printf("epoch %d: computer2 wins\n", i);
			env.render(false);
			printf("epoch %d: winning rate = %.2f%%, time = %.2fs\n", i, 
				100.0 * win / i, 1.0 * (clock() - start) / CLOCKS_PER_SEC);
			printf("-----------------------------------------------------------------------\n");
			pos = Position();
			computer1.clear();
			env.reset();
		}
		printf("winning rate = %f%%\n", 100.0 * win / total);
	}
	system("pause");
	return 0;
}
