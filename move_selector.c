#include "move_selector.h"
#include "move_generator.h"
#include "utility.h"
#include "evaluation.h"

static void scoreMoves(const ChessBoard *restrict board, MoveSelector *restrict ms) {
    MoveObject *startList = ms->startList;
    while (startList < ms->endList) {
        PieceType capturedPiece = board->pieceTypes[getToSquare(startList->move)];
        if (capturedPiece) {
            startList->score = PIECE_VALUE[capturedPiece] - board->pieceTypes[getFromSquare(startList->move)]; // MVV/LVA
        } else if (getMoveType(startList->move) & EN_PASSANT) {
            startList->score = 90;
        } else if (startList->move == ms->killers[0]) {
            startList->score = 51;
        } else if (startList->move == ms->killers[1]) {
            startList->score = 50;
        } else {
            startList->score = 0;
        }
        startList++;
    }
}

static Move getNextHighestScoringMove(MoveSelector *ms) {
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

Move getNextBestMove(const ChessBoard *restrict board, MoveSelector *restrict ms) {
    while (true) {
        switch (ms->state) {
            case TT_MOVE:
                ms->state++;
                if (isPseudoMove(board, ms->ttMove)) return ms->ttMove;
                break;
            case ALL_MOVES_INIT:
                ms->state++;
                ms->endList = createMoveList(board, ms->moveList, CAPTURES);
                ms->endList = createMoveList(board, ms->endList, NON_CAPTURES);
                scoreMoves(board, ms);
                break;
            case ALL_MOVES:
                return getNextHighestScoringMove(ms);
            case TEMP:
                return NO_MOVE;
        }
    }
}
