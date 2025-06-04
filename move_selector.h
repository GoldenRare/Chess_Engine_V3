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
    Move killers[2];
} MoveSelector;

static inline void createMoveSelector(MoveSelector *restrict ms, MoveSelectorState state, Move ttMove, const Move *restrict killers) {
    ms->state = state;
    ms->ttMove = ttMove;
    ms->startList = ms->moveList;
    ms->killers[0] = killers[0];
    ms->killers[1] = killers[1];
}

inline void swap(MoveObject *mo1, MoveObject *mo2) {
    MoveObject temp = *mo1;
    *mo1 = *mo2;
    *mo2 = temp;
}

Move getNextBestMove(const ChessBoard *board, MoveSelector *ms);

#endif