#include "attacks.h"
#include "chess_board.h"
#include "uci.h"

int main () {
    initializeAttacks();
    initializeChessBoard();
    uciLoop();
    return 0;
}
