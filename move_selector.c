#include "move_selector.h"
#include "move_generator.h"
#include "utility.h"
#include "evaluation.h"

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
                scoreMoves(board, ms);
                break;
            case ALL_MOVES:
                return getNextHighestScoringMove(ms);
            case TEMP:
                return NO_MOVE;
        }
    }
}

Move getNextHighestScoringMove(MoveSelector *ms) {
    if (ms->startList->move == ms->ttMove) ms->startList++; // TODO: Test getting rid of this check, and simply just restarting another search
    if (ms->startList >= ms->endList) return NO_MOVE;

    MoveObject *highestScoreMove = ms->startList;
    for (MoveObject *moveObj = ms->startList + 1; moveObj < ms->endList; moveObj++) {
        if (moveObj->move == ms->ttMove) continue;
        if (moveObj->score > highestScoreMove->score) highestScoreMove = moveObj;
    }

    if (highestScoreMove != ms->startList) swap(highestScoreMove, ms->startList);
    return ms->startList++->move; 
}

void scoreMoves(const ChessBoard *board, MoveSelector *ms) {
    MoveObject *startList = ms->startList;
    while (startList < ms->endList) {
        MoveType moveType = getMoveType(&startList->move);
        if (moveType & CAPTURE) {
            startList->score = moveType == EN_PASSANT_CAPTURE ? 1 
                             : PIECE_VALUE[board->pieceTypes[getToSquare(&startList->move)]] - board->pieceTypes[getFromSquare(&startList->move)];
        } else {
            startList->score = 0;
        }
        startList++;
    }
}