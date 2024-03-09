#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "search.h"
#include "chess_board.h"
#include "utility.h"
#include "move_generator.h"
#include "evaluation.h"
#include "transposition_table.h"
#include "move_sorter.h"

void startSearch(ChessBoard *board, int depth) {
    SearchHelper sh[MAX_DEPTH + 1]; //TODO: SIZE
    Move pv[MAX_DEPTH + 1];
    sh[0].pv = pv;
    pv[0] = NO_MOVE;
    
    char pvString[2048];
    double totalTime;
    clock_t start = clock();
    for (int d = 1; d <= depth; d++) { // TODO: Experiment with different steps for iterative deepening
        int score = alphaBeta(board, -INFINITE, INFINITE, d, sh, true);
        totalTime = (double) (clock() - start) / CLOCKS_PER_SEC;

        encodePrincipalVariation(pvString, pv);
        // TODO: Should eventually include seldepth, nodes, multipv?, score mate, nps, maybe others 
        printf("info depth %d time %.0lf score cp %d pv %s\n", d, totalTime * 1000.0, score, pvString);
    }
    printf("bestmove %s\n", pvString); //TODO: Should eventually include ponder
}

int alphaBeta(ChessBoard *board, int alpha, int beta, int depth, SearchHelper *sh, bool isRootNode) {
    /* Quiescence Search */
    if (depth == 0) return quiescenceSearch(board, alpha, beta);
    /*                   */

    /* Transposition Table */
    bool hasEvaluation;
    PositionEvaluation *pe = probeTranspositionTable(board->positionKey, &hasEvaluation);
    Move ttMove = NO_MOVE;
    if (hasEvaluation) {
        Bound bound = getBound(pe);
        int nodeScore = pe->nodeScore;
        if (pe->depth >= depth && !isRootNode) { // Do not terminate early if root node so that we can at least report one move in the pv for 'info string'
            // TODO: Consider optimizing the below and need to adjust node score
            if ((bound == LOWER && nodeScore >= beta) || (bound == UPPER && nodeScore <= alpha) || bound == EXACT) {
                return nodeScore; // Fail Soft
            }
        }
        ttMove = pe->bestMove; 
    }
    /*                     */

    /* Main Moves Loop */
    Move childrenPv[depth];
    sh[1].pv = childrenPv;
    childrenPv[0] = NO_MOVE;

    IrreversibleBoardState ibs;
    Move moveList[256];
    Move *endList = createMoveList(board, moveList);
    sortMoves(moveList, endList, ttMove);
    Bitboard pinnedPieces = getPinnedPieces(board);

    int bestScore = -INFINITE;
    Move bestMove = NO_MOVE;
    for (Move *move = moveList; move != endList; move++) {
        if (!isLegalMove(board, move, pinnedPieces)) continue;
        makeMove(board, move, &ibs);
        int score = -alphaBeta(board, -beta, -alpha, depth - 1, sh + 1, false);
        undoMove(board, move, &ibs);

        // bestScore <= alpha < beta
        if (score > bestScore) {
            bestScore = score;
            if (score > alpha) {
                if (score >= beta) {
                    savePositionEvaluation(pe, board->positionKey, *move, depth, LOWER, score);
                    return score; // Fail Soft
                }
                updatePrincipalVariation(*move, sh->pv, childrenPv);
                bestMove = *move;
                alpha = score; 
            }
        }
    }
    /*                 */

    /* Checkmate and Stalemate Detection */
    if (bestScore == -INFINITE) { // Something is always better than nothing, so bestScore stays -INFINITE if it couldn't play any move
        sh->pv[0] = NO_MOVE; // TODO: Likely clean this up
        Colour stm = board->sideToMove;
        bestScore = attackersTo(board, getKingSquare(board, stm), stm, getOccupiedSquares(board)) ? CHECKMATED - depth : DRAW; // TODO: Temporary solution, should this be considered EXACT bound?
    }
    /*                                   */

    savePositionEvaluation(pe, board->positionKey, bestMove, depth, bestMove != NO_MOVE ? EXACT : UPPER, bestScore);
    return bestScore; // Fail Soft
}

int quiescenceSearch(ChessBoard *board, int alpha, int beta) {
    Move moveList[256];
    Move *endList = createMoveList(board, moveList);
    Bitboard pinnedPieces = getPinnedPieces(board);
    for (Move *move = moveList; move != endList; move++) 
        if (isLegalMove(board, move, pinnedPieces)) return evaluation(board); // TODO: Not a true qsearch, we are just trying to ensure that we don't do a static evaluation for a checkmate position
    Colour stm = board->sideToMove;
    return attackersTo(board, getKingSquare(board, stm), stm, getOccupiedSquares(board)) ? CHECKMATED : DRAW; // TODO: Temporary solution, should this be considered EXACT bound?
} 

void encodePrincipalVariation(char* buffer, const Move *pv) {
    int spaceIndex = 0;
    for (const Move *m = pv; *m != NO_MOVE; m++) {
        MoveType moveType = getMoveType(m);
        char promotionPiece = moveType & PROMOTION ? PROMOTION_NAME[moveType & PROMOTION_PIECE_OFFSET_MASK] : '\0';
        encodeChessMove(buffer, SQUARE_NAME[getFromSquare(m)], SQUARE_NAME[getToSquare(m)], promotionPiece);
        spaceIndex = buffer[4] == '\0' ? 4 : 5;
        buffer[spaceIndex] = ' ';
        buffer = buffer + spaceIndex + 1;
    }
    buffer[spaceIndex] = '\0';
}