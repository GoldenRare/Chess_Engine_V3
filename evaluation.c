#include "evaluation.h"
#include "chess_board.h"
#include "utility.h"

int evaluation(const ChessBoard *board) {
    int score = 9 * (populationCount(getPieces(board, WHITE, QUEEN )) - populationCount(getPieces(board, BLACK, QUEEN )))
              + 5 * (populationCount(getPieces(board, WHITE, ROOK  )) - populationCount(getPieces(board, BLACK, ROOK  )))
              + 3 * (populationCount(getPieces(board, WHITE, BISHOP)) - populationCount(getPieces(board, BLACK, BISHOP)))
              + 3 * (populationCount(getPieces(board, WHITE, KNIGHT)) - populationCount(getPieces(board, BLACK, KNIGHT)))
              +     (populationCount(getPieces(board, WHITE, PAWN  )) - populationCount(getPieces(board, BLACK, PAWN  )));
    return board->sideToMove ? -score : score;
}