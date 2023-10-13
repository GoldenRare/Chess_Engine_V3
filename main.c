#include "move_generator.h"
#include "chess_board.h"
#include "uci.h"

int main () {
    initializeMoveGenerator();
    initializeChessBoard();
    uciLoop();
    return 0;
}