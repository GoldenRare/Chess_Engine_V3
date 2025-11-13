#ifndef MOVE_GENERATOR_H
#define MOVE_GENERATOR_H

#include "chess_board.h"
#include "utility.h"

// TODO: Legal move generation stage
typedef enum MoveGenerationStage {
    CAPTURES, NON_CAPTURES, LEGAL
} MoveGenerationStage;

MoveObject* createMoveList(const ChessBoard *restrict board, MoveObject *restrict moveList, MoveGenerationStage stage);
MoveObject* generateCastleMoves(const ChessBoard *restrict board, MoveObject *restrict moveList);
bool anyLegalMoves(const ChessBoard *restrict board);

#endif
