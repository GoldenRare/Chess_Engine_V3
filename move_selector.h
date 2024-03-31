#ifndef MOVE_SELECTOR_H
#define MOVE_SELECTOR_H

#include "chess_board.h"
#include "utility.h"

enum MoveSelectorState {
    TT_MOVE, ALL_MOVES_INIT, ALL_MOVES, TEMP
};
typedef enum MoveSelectorState MoveSelectorState;

typedef struct MoveSelector {
    MoveObject *startList;
    MoveObject *endList;
    MoveObject moveList[256];
    MoveSelectorState state;
    Move ttMove;
} MoveSelector;

inline void createMoveSelector(MoveSelector *ms, MoveSelectorState state, Move ttMove) {
    ms->state = state;
    ms->ttMove = ttMove;
    ms->startList = ms->moveList;
}

inline void swap(MoveObject *mo1, MoveObject *mo2) {
    MoveObject temp = *mo1;
    *mo1 = *mo2;
    *mo2 = temp;
}

Move getNextBestMove(const ChessBoard *board, MoveSelector *ms);
Move getNextHighestScoringMove(MoveSelector *ms);

void scoreMoves(const ChessBoard *board, MoveSelector *ms);

#endif