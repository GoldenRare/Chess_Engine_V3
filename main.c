#include "move_generator.h"
#include "chess_board.h"
#include "uci.h"
#include "nnue.h"

int main () {
    initializeMoveGenerator();
    initializeChessBoard();
    initializeNNUE();
    uciLoop();
    return 0;
}
