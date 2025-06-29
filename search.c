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
    Move* pv; // The Principal Variation
    uint8_t ply;
} SearchHelper;

static inline void updatePrincipalVariation(Move m, Move *currentPv, const Move *childrenPv) {
    *currentPv++ = m;
    while((*currentPv++ = *childrenPv++) != NO_MOVE);
}

static void encodePrincipalVariation(char* buffer, const Move *pv) {
    int spaceIndex = 0;
    for (const Move *m = pv; *m != NO_MOVE; m++) {
        MoveType moveType = getMoveType(*m);
        char promotionPiece = moveType & PROMOTION ? PROMOTION_NAME[moveType & PROMOTION_PIECE_OFFSET_MASK] : '\0';
        encodeChessMove(buffer, SQUARE_NAME[getFromSquare(*m)], SQUARE_NAME[getToSquare(*m)], promotionPiece);
        spaceIndex = buffer[4] == '\0' ? 4 : 5;
        buffer[spaceIndex] = ' ';
        buffer = buffer + spaceIndex + 1;
    }
    buffer[spaceIndex] = '\0';
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

    /* Quiescence Search */
    if (!depth) return quiescenceSearch(board, alpha, beta, sh);
    /*                   */

    if (isDraw(board)) return DRAW;

    Key positionKey = getPositionKey(board);
    /* Transposition Table */
    /*bool hasEvaluation;
    PositionEvaluation *pe = probeTranspositionTable(positionKey, &hasEvaluation);
    Move ttMove = NO_MOVE;
    if (hasEvaluation) {
        Bound bound = getBound(pe);
        Score nodeScore = adjustNodeScoreFromTT(pe->nodeScore, sh->ply);
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
    sh[1].ply = sh->ply + 1;
    sh[1].pv = childrenPv;
    childrenPv[0] = NO_MOVE;

    ChessBoardHistory history;
    MoveSelector ms;
    createMoveSelector(&ms, board, TT_MOVE, NO_MOVE);

    Score bestScore = -INFINITE;
    Move bestMove = NO_MOVE;
    Move move;
    while ((move = getNextBestMove(board, &ms))) {
        if (!isLegalMove(board, move)) continue;
        makeMove(board, &history, move);
        Score score = -alphaBeta(board, -beta, -alpha, depth - 1, sh + 1, false);
        undoMove(board, move);

        if (score > bestScore) {
            if (score > alpha) {
                if (score >= beta) {
                    //savePositionEvaluation(pe, positionKey, move, depth, LOWER, adjustNodeScoreToTT(score, sh->ply));
                    return score;
                }
                updatePrincipalVariation(move, sh->pv, childrenPv);
                bestMove = move;
                alpha = score; 
            }
            bestScore = score;
        }
    }
    /*                 */

    /* Checkmate and Stalemate Detection */
    if (bestScore == -INFINITE) {
        sh->pv[0] = NO_MOVE; // TODO: Likely clean this up
        bestScore = getCheckers(board) ? -CHECKMATE + sh->ply : DRAW; // TODO: Should this be considered EXACT bound?
    }
    /*                                   */

    //savePositionEvaluation(pe, positionKey, bestMove, depth, bestMove != NO_MOVE ? EXACT : UPPER, adjustNodeScoreToTT(bestScore, sh->ply));
    return bestScore;
}

MoveObject startSearch(ChessBoard *restrict board, Depth depth) {
    startNewSearch();
    SearchHelper sh[MAX_DEPTH + 1];
    Move pv[MAX_DEPTH + 1];
    sh[0].ply = 0;
    sh[0].pv = pv;
    pv[0] = NO_MOVE;
    
    char pvString[2048];
    double totalTime;
    int score = 0;
    clock_t start = clock();
    for (int d = 1; d <= depth; d++) { // TODO: Experiment with different steps for iterative deepening
        score = alphaBeta(board, -INFINITE, INFINITE, d, sh, true);
        totalTime = (double) (clock() - start) / CLOCKS_PER_SEC;

        encodePrincipalVariation(pvString, pv);
        // TODO: Should eventually include seldepth, nodes, multipv?, score mate, nps, maybe others 
        printf("info depth %d time %.0lf score cp %d pv %s\n", d, totalTime * 1000.0, score, pvString);
    }
    printf("bestmove %s\n", pvString); //TODO: Should eventually include ponder
    MoveObject best = {pv[0], score};
    return best;
}
