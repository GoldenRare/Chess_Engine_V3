#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "search.h"
#include "chess_board.h"
#include "utility.h"
#include "move_generator.h"
#include "evaluation.h"
#include "transposition_table.h"

void startSearch(ChessBoard *board, int depth) {
    int bestScore = -INFINITE;
    Move *bestMove = NULL;
    
    clock_t start = clock();
    IrreversibleBoardState ibs;
    Move moveList[256];
    Move *endList = createMoveList(board, moveList);
    Bitboard pinnedPieces = getPinnedPieces(board);
    for (Move *move = moveList; move != endList; move++) {
        if (!isLegalMove(board, move, pinnedPieces)) continue;
        makeMove(board, move, &ibs);
        int score = -alphaBeta(board, -INFINITE, -bestScore, depth - 1);
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        } 
        undoMove(board, move, &ibs);
    }
    clock_t end = clock();
    double totalTime = (double) (end - start) / CLOCKS_PER_SEC;

    char moveStr[6];
    MoveType moveType = getMoveType(bestMove);
    char promotionPiece = moveType & PROMOTION ? PROMOTION_NAME[moveType & PROMOTION_PIECE_OFFSET_MASK] : '\0';
    encodeChessMove(moveStr, SQUARE_NAME[getFromSquare(bestMove)], SQUARE_NAME[getToSquare(bestMove)], promotionPiece);

    // Should eventually include seldepth, nodes, multipv?, score mate, nps, maybe others 
    printf("info depth %d time %.0lf score cp %d pv %s\n", depth, totalTime * 1000.0, bestScore, moveStr);
    printf("bestmove %s\n", moveStr); // Should eventually include ponder
}

int alphaBeta(ChessBoard *board, int alpha, int beta, int depth) {
    /* Transposition Table */
    bool hasEvaluation;
    PositionEvaluation *pe = probeTranspositionTable(board->positionKey, &hasEvaluation);
    if (hasEvaluation) {
        Bound bound = getBound(pe);
        int nodeScore = pe->nodeScore;
        if (pe->depth >= depth) {
            // TODO: Consider optimizing the below and need to adjust node score
            if ((bound == LOWER && nodeScore >= beta) || (bound == UPPER && nodeScore <= alpha) || bound == EXACT) {
                return nodeScore; // Fail Soft
            }
        } 
    }
    /*                     */

    /* Quiescence Search */
    if (depth == 0) return evaluation(board); //TODO: Test this before TT probe
    /*                   */

    /* Main Moves Loop */
    IrreversibleBoardState ibs;
    Move moveList[256];
    Move *endList = createMoveList(board, moveList);
    Bitboard pinnedPieces = getPinnedPieces(board);

    int bestScore = -INFINITE;
    Move bestMove = NO_MOVE;
    for (Move *move = moveList; move != endList; move++) {
        if (!isLegalMove(board, move, pinnedPieces)) continue;
        makeMove(board, move, &ibs);
        int score = -alphaBeta(board, -beta, -alpha, depth - 1);
        undoMove(board, move, &ibs);

        // bestScore <= alpha < beta
        if (score > bestScore) {
            bestScore = score;
            if (score > alpha) {
                if (score >= beta) {
                    savePositionEvaluation(pe, board->positionKey, *move, depth, LOWER, score);
                    return score; // Fail Soft
                }
                bestMove = *move;
                alpha = score; 
            }
        }
    }
    /*                 */

    /* Checkmate and Stalemate Detection */
    if (bestScore == -INFINITE) { // Something is always better than nothing, so bestScore stays -INFINITE if it couldn't play any move
        Colour stm = board->sideToMove;
        bestScore = attackersTo(board, getKingSquare(board, stm), stm, getOccupiedSquares(board)) ? CHECKMATED - depth : DRAW; // TODO: Temporary solution, should this be considered EXACT bound?
    }
    /*                                   */

    savePositionEvaluation(pe, board->positionKey, bestMove, depth, bestMove != NO_MOVE ? EXACT : UPPER, bestScore);
    return bestScore; // Fail Soft
}