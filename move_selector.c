#include "move_selector.h"
#include "move_generator.h"
#include "utility.h"

Move getNextBestMove(const ChessBoard *board, MoveSelector *ms) {
    while (true) {
        switch (ms->state) {
            case TT_MOVE:
                ms->state++;
                if (ms->ttMove && isPseudoMove(board, ms->ttMove)) return ms->ttMove;
                break;
            case ALL_MOVES_INIT:
                ms->state++;
                ms->endList = createMoveList(board, ms->moveList);
                break;
            case ALL_MOVES:
                while (ms->startList < ms->endList) {
                    if (*ms->startList != ms->ttMove) return *ms->startList++;
                    ms->startList++;
                }
                return NO_MOVE;
        }
    }
}