#include "move_selector.h"
#include "chess_board.h"
#include "move_generator.h"
#include "utility.h"

Move* createMoveList(const ChessBoard *board, Move *moveList) {
    Square kingSq = getKingSquare(board, board->sideToMove);
    Bitboard checkers = isSquareAttacked(board, kingSq, board->sideToMove);
    if (!checkers) {
        return generateAllMoves(board, moveList, ENTIRE_BOARD);
    } else if (populationCount(checkers) == 1) {
        return generateAllMoves(board, moveList, inBetweenLine[kingSq][bitboardToSquare(checkers)] | checkers);
    } else {
        return generateKingMoves(board, moveList);
    }
}