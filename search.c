#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "search.h"
#include "chess_board.h"
#include "utility.h"
#include "transposition_table.h"
#include "move_selector.h"
#include "nnue.h"

typedef struct SearchHelper {
    Move pv[MAX_DEPTH]; // TODO: Is it worth saving space by making triangular?
    uint8_t ply;
} SearchHelper;

static inline void updatePV(Move move, Move *restrict currentPV, const Move *restrict childrenPV) {
    *currentPV++ = move;
    while((*currentPV++ = *childrenPV++));
}

static inline void pvToString(char *restrict destination, const Move *restrict pv) {
    for (Depth depth = 0; depth < MAX_DEPTH && pv[depth]; depth++) {
        if (depth) *destination++ = ' ';
        destination += moveToString(destination, pv[depth]);
    }
}

static Score quiescenceSearch(ChessBoard *restrict board, Score alpha, Score beta, SearchHelper *restrict sh) {

    if (isDraw(board)) return DRAW;
    
    Bitboard checkers = getCheckers(board);
    /* Stand Pat */
    Score bestScore = checkers ? -CHECKMATE + sh->ply : evaluation(board->sideToMove); // TODO: Could be evaluating a stalemate
    if (bestScore > alpha) {
        if (bestScore >= beta) return bestScore; 
        alpha = bestScore;
    }
    /*           */

    uint8_t ply = sh->ply;
    /* Main Moves Loop */
    ChessBoardHistory history;
    MoveSelector ms;
    MoveSelectorState state = checkers ? TT_MOVE : GET_NON_CAPTURE_MOVES; // TODO: Cleanup naming
    createMoveSelector(&ms, board, state, NO_MOVE);

    Move move;
    while ((move = getNextBestMove(board, &ms))) {
        if (!isLegalMove(board, move)) continue;
        sh->ply = ply + 1;
        makeMove(board, &history, move);
        Score score = -quiescenceSearch(board, -beta, -alpha, sh);
        undoMove(board, move);
        sh->ply = ply;
        
        if (score > bestScore) {
            if (score > alpha) {
                if (score >= beta) return score;
                alpha = score; 
            }
            bestScore = score;
        }
    }
    /*                 */
    return bestScore;
}

static Score alphaBeta(ChessBoard *restrict board, Score alpha, Score beta, Depth depth, SearchHelper *restrict sh, bool isRootNode) {
    sh->pv[0] = NO_MOVE;

    /* 1) Quiescence Search */
    if (!depth) return quiescenceSearch(board, alpha, beta, sh);
    /*                      */
    
    /* 2) Draw Detection */
    if (isDraw(board)) return DRAW;
    /*                   */

    /* 3) Transposition Table */
    Key positionKey = getPositionKey(board);
    bool hasEvaluation;
    PositionEvaluation *pe = probeTranspositionTable(positionKey, &hasEvaluation);
    Move ttMove = NO_MOVE;
    if (hasEvaluation) {
        // TODO: Do not terminate early if root node so that we can at least report one move in the pv for 'info string'
        if (pe->depth >= depth && !isRootNode) {
            Bound bound = getBound(pe); // TODO: Should you extract earlier due to race conditions?
            Score nodeScore = adjustNodeScoreFromTT(pe->nodeScore, sh->ply);
            // TODO: Consider optimizing the below
            if ((bound == LOWER && nodeScore >= beta) || (bound == UPPER && nodeScore <= alpha) || bound == EXACT) return nodeScore;
        }
        ttMove = pe->bestMove;
    }
    /*                        */

    /* Main Moves Loop */
    SearchHelper *child = sh + 1;
    child->ply = sh->ply + 1;

    ChessBoardHistory history;
    MoveSelector ms;
    createMoveSelector(&ms, board, TT_MOVE, ttMove);

    Score bestScore = -INFINITE;
    Move  bestMove  =   NO_MOVE, move;
    while ((move = getNextBestMove(board, &ms))) {
        if (!isLegalMove(board, move)) continue;
        makeMove(board, &history, move);
        Score score = -alphaBeta(board, -beta, -alpha, depth - 1, child, false);
        undoMove(board, move);

        if (score > bestScore) {
            if (score > alpha) {
                if (score >= beta) {
                    savePositionEvaluation(pe, positionKey, move, depth, LOWER, adjustNodeScoreToTT(score, sh->ply));
                    return score;
                }
                updatePV(move, sh->pv, child->pv); // TODO: Only needs to be done once on the last score > alpha, but integrity is lost
                bestMove = move; // TODO: Consider doing it when score > bestScore, also would need to change savePosEval() below
                alpha = score; 
            }
            bestScore = score;
        }
    }
    /*                 */

    /* Checkmate and Stalemate Detection */
    if (bestScore == -INFINITE) bestScore = getCheckers(board) ? -CHECKMATE + sh->ply : DRAW; // TODO: Should this be considered EXACT bound?
    /*                                   */

    savePositionEvaluation(pe, positionKey, bestMove, depth, bestMove != NO_MOVE ? EXACT : UPPER, adjustNodeScoreToTT(bestScore, sh->ply));
    return bestScore;
}

MoveObject startSearch(ChessBoard *restrict board, Depth depth) {
    startNewSearch();
    SearchHelper sh[MAX_DEPTH + 1];
    sh[0].ply = 0;
    
    char pvString[2048];
    double totalTime;
    int score = 0;
    clock_t start = clock();
    for (int d = 1; d <= depth; d++) { // TODO: Experiment with different steps for iterative deepening
        score = alphaBeta(board, -INFINITE, INFINITE, d, sh, true);
        totalTime = (double) (clock() - start) / CLOCKS_PER_SEC;

        pvToString(pvString, sh[0].pv);
        // TODO: Should eventually include seldepth, nodes, multipv?, score mate, nps, maybe others 
        printf("info depth %d time %.0lf score cp %d pv %s\n", d, totalTime * 1000.0, score, pvString);
    }
    printf("bestmove %s\n", pvString); //TODO: Should eventually include ponder
    MoveObject best = {sh[0].pv[0], score};
    return best;
}
