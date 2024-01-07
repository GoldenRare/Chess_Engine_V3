#include "move_generator.h"
#include "chess_board.h"
#include "uci.h"
#include "zobrist.h"

int main () {
    initializeMoveGenerator();
    initializeChessBoard();
    initializeZobrist();
    uciLoop();
    return 0;
}