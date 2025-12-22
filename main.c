#include "attacks.h"
#include "chess_board.h"
#include "nnue.h"
#include "uci.h"

int main () {
    initializeAttacks();
    initializeChessBoard();
    initializeNNUE();
    uciLoop();
    return 0;
}
