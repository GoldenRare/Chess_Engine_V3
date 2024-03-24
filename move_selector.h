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

Move getNextBestMove(const ChessBoard *board, MoveSelector *ms);
void scoreMoves(MoveSelector *ms);

#endif