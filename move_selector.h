#ifndef MOVE_SELECTOR_H
#define MOVE_SELECTOR_H

#include "chess_board.h"
#include "utility.h"

typedef enum MoveSelectorState {
    TT_MOVE, CAPTURE_MOVES, GET_CAPTURES, NON_CAPTURE_MOVES, GET_NON_CAPTURE_MOVES, TEMP
} MoveSelectorState;

typedef struct MoveSelector {
    MoveObject *startList;
    MoveObject *endList;
    MoveSelectorState state;
    MoveObject moveList[256];
    Move ttMove;
} MoveSelector;

static inline void createMoveSelector(MoveSelector *restrict ms, const ChessBoard *restrict board, MoveSelectorState state, Move ttMove) {
    ms->state = state + !(ttMove && isPseudoMove(board, ttMove));
    ms->ttMove = ttMove;
    ms->startList = ms->moveList;
}

Move getNextBestMove(const ChessBoard *board, MoveSelector *ms);

#endif
