#include "move_selector.h"
#include "chess_board.h"
#include "move_generator.h"
#include "utility.h"

Move* createMoveList(const ChessBoard *board, Move *moveList) {
    Colour stm = board->sideToMove;
    Square kingSq = getKingSquare(board, stm);
    Bitboard checkers = isSquareAttacked(board, kingSq, stm);
    if (!checkers) {
        Bitboard validSquares = ENTIRE_BOARD & ~getPieces(board, stm, ALL_PIECES);
        return generateAllMoves(board, moveList, validSquares);
    } else if (populationCount(checkers) == 1) {
        Bitboard validSquares = (inBetweenLine[kingSq][bitboardToSquare(checkers)] | checkers) & ~getPieces(board, stm, ALL_PIECES);
        return generateAllMoves(board, moveList, validSquares);
    } else {
        return generateKingMoves(board, moveList);
    }
}