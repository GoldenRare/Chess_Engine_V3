#ifndef MOVE_SELECTOR_H
#define MOVE_SELECTOR_H

#include "chess_board.h"
#include "utility.h"

enum MoveSelectorState {
    TT_MOVE, ALL_MOVES_INIT, ALL_MOVES
};
typedef enum MoveSelectorState MoveSelectorState;

typedef struct MoveSelector {
    Move *startList;
    Move *endList;
    MoveSelectorState state;
    Move ttMove;
    Move moveList[256];
} MoveSelector;

inline void createMoveSelector(MoveSelector *ms, MoveSelectorState state, Move ttMove) {
    ms->state = state;
    ms->ttMove = ttMove;
    ms->startList = ms->moveList;
}

Move getNextBestMove(const ChessBoard *board, MoveSelector *ms);

#endif