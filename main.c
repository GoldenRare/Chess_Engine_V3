#include "move_generator.h"
#include "chess_board.h"
#include "uci.h"
#include "nnue.h"
#include "transposition_table.h"
#include "training.h"

int main () {
    initializeMoveGenerator();
    initializeChessBoard();
    initializeNNUE();
    initializeTranspositionTable();
    initializeTraining();
    uciLoop();
    return 0;
}
