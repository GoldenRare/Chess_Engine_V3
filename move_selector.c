#include "move_selector.h"
#include "move_generator.h"
#include "utility.h"

constexpr Score PIECE_VALUE[PIECE_TYPES] = {0, 100, 300, 306, 500, 900, 0};

static inline void swap(MoveObject *mo1, MoveObject *mo2) {
    MoveObject temp = *mo1;
    *mo1 = *mo2;
    *mo2 = temp;
}

static void scoreMoves(const ChessBoard *restrict board, MoveSelector *restrict ms) {
    MoveObject *startList = ms->startList;
    while (startList < ms->endList) {
        PieceType capturedPiece = board->pieceTypes[getToSquare(startList->move)];
        if (capturedPiece) {
            startList->score = PIECE_VALUE[capturedPiece] - board->pieceTypes[getFromSquare(startList->move)]; // MVV/LVA
        } else if (getMoveType(startList->move) & EN_PASSANT) {
            startList->score = 90;
        } else {
            startList->score = 0;
        }
        startList++;
    }
}

static Move getNextHighestScoringMove(MoveSelector *restrict ms) {
    MoveObject *highestScoreMove = &(MoveObject) {NO_MOVE, -1}; // TODO: Score must be the lowest possible
    for (MoveObject *moveObj = ms->startList; moveObj < ms->endList; moveObj++)
        if (moveObj->move != ms->ttMove && moveObj->score > highestScoreMove->score) highestScoreMove = moveObj;
    Move bestMove = highestScoreMove->move;
    if (highestScoreMove->score != -1) swap(highestScoreMove, ms->startList++);
    return bestMove;
}

Move getNextBestMove(const ChessBoard *restrict board, MoveSelector *restrict ms) {
    while (true) {
        switch (ms->state) {
            case TT_MOVE:
                ms->state++;
                return ms->ttMove;
            case CAPTURE_MOVES:
                ms->state++;
                ms->endList = createMoveList(board, ms->moveList, CAPTURES);
                scoreMoves(board, ms);
                break;
            case GET_CAPTURES:
                Move m;
                if ((m = getNextHighestScoringMove(ms))) return m;
                ms->state++;
                break;
            case NON_CAPTURE_MOVES:
                ms->state++;
                ms->startList = ms->endList;
                ms->endList = createMoveList(board, ms->endList, NON_CAPTURES);
                scoreMoves(board, ms);
                break;
            case GET_NON_CAPTURE_MOVES:
                return getNextHighestScoringMove(ms);
            case TEMP:
            default:
                return NO_MOVE;
        }
    }
}
