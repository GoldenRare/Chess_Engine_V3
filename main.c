#include "move_generator.h"
#include "chess_board.h"
#include "uci.h"
#include "nnue.h"
#include "transposition_table.h"

int main () {
    initializeMoveGenerator();
    initializeChessBoard();
    initializeNNUE();
    initializeTranspositionTable();
    uciLoop();
    return 0;
}
