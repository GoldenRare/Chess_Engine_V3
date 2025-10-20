#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "search.h"
#include "chess_board.h"
#include "utility.h"
#include "transposition_table.h"
#include "move_selector.h"
#include "nnue.h"

constexpr Depth MAX_DEPTH = 255;
static atomic_bool stop;

typedef struct SearchHelper {
    Move pv[MAX_DEPTH]; // TODO: Is it worth saving space by making triangular?
    uint8_t ply;
} SearchHelper;

static inline void updatePV(Move move, Move *restrict currentPV, const Move *restrict childrenPV) {
    *currentPV++ = move;
    while((*currentPV++ = *childrenPV++));
}

// TODO: Is this the ideal implementation?
// Always assumes there is at least one move in the principal variation
static inline void pvToString(char *restrict pvStr, char *restrict bestMove, char *restrict ponderMove, const Move *restrict pv) {
    moveToString(bestMove, pv[0]);
    pvStr += moveToString(pvStr, pv[0]);
    *ponderMove = '\0';
    for (Depth depth = 1; depth < MAX_DEPTH && pv[depth]; depth++) {
        if (depth == 1) moveToString(ponderMove, pv[depth]);
        *pvStr++ = ' ';
        pvStr += moveToString(pvStr, pv[depth]);
    }
}

/*static inline uint64_t timeDiffMs(const struct timespec *restrict start, const struct timespec *restrict end) {
    time_t diffsecs = end->tv_sec - start->tv_sec;
    long   diffnano = end->tv_nsec - end->tv_nsec;
    if (diffnano < 0) {
        diffsecs--;
        diffnano += 1000000000L;
    }
    return (long long)diffsecs * 1000LL + (long long)(diffnano / 1000000L);
}*/

static Score quiescenceSearch(ChessBoard *restrict board, Score alpha, Score beta, SearchHelper *restrict sh) {

    if (isDraw(board)) return DRAW;
    
    Bitboard checkers = getCheckers(board);
    /* Stand Pat */
    Score bestScore = checkers ? -CHECKMATE + sh->ply : evaluation(&board->accumulator, board->sideToMove); // TODO: Could be evaluating a stalemate
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

// Does not terminate early if root node so that we can at least report one move in the pv for 'info string'
static Score alphaBeta(Score alpha, Score beta, Depth depth, SearchHelper *restrict sh, bool isRootNode, SearchThread *st) {
    ChessBoard *board = &st->board;
    sh->pv[0] = NO_MOVE;

    /* 1) Quiescence Search */
    if (!depth) return quiescenceSearch(board, alpha, beta, sh);
    /*                      */
    
    /* 2) Draw Detection */
    if (!isRootNode && isDraw(board)) return DRAW;
    /*                   */

    /* 3) Out of Time Check */
    if (atomic_load_explicit(&stop, memory_order_relaxed)) return DRAW; // TODO: Is this expensive?
    /*                      */

    /* 4) Transposition Table */
    Key positionKey = getPositionKey(board);
    bool hasEvaluation;
    PositionEvaluation *pe = probeTranspositionTable(st->tt, positionKey, &hasEvaluation);
    Move ttMove = NO_MOVE;
    if (hasEvaluation) {
        if (!isRootNode && pe->depth >= depth) {
            Bound bound = getBound(pe); // TODO: Should you extract earlier due to race conditions?
            Score nodeScore = adjustNodeScoreFromTT(pe->nodeScore, sh->ply);
            // TODO: Consider optimizing the below
            if ((bound == LOWER && nodeScore >= beta) || (bound == UPPER && nodeScore <= alpha) || bound == EXACT) return nodeScore;
        }
        ttMove = pe->bestMove;
    }
    /*                        */

    SearchHelper *child = sh + 1;
    child->ply = sh->ply + 1;

    ChessBoardHistory history;
    MoveSelector ms;
    createMoveSelector(&ms, board, TT_MOVE, ttMove);

    Score bestScore = -INFINITE, oldAlpha = alpha;
    Move  bestMove  =   NO_MOVE, move;
    bool isPvNode = beta - alpha > 1, isFirstMove = true;

    /* 5) Move Ordering */
    while ((move = getNextBestMove(board, &ms))) {
        if (!isLegalMove(board, move)) continue;
        makeMove(board, &history, move);

        /* 6) Principal Variation Search */
        Score score;
        if (!isPvNode || !isFirstMove) score = -alphaBeta(-alpha - 1, -alpha, depth - 1, child, false, st);
        if (isPvNode && (isFirstMove || (score > alpha && score < beta))) score = -alphaBeta(-beta, -alpha, depth - 1, child, false, st);
        /*                               */
        
        undoMove(board, move);

        if (score > bestScore) {
            if (score > alpha) {
                if (score >= beta) {
                    if (!atomic_load_explicit(&stop, memory_order_relaxed)) savePositionEvaluation(st->tt, pe, positionKey, move, depth, LOWER, adjustNodeScoreToTT(score, sh->ply));
                    return score;
                }
                updatePV(move, sh->pv, child->pv); // TODO: Only needs to be done once on the last score > alpha, but integrity is lost
                alpha = score; 
            }
            bestScore = score;
            bestMove = move;
        }
        isFirstMove = false;
    }
    /*                  */

    /* Checkmate and Stalemate Detection */
    if (bestScore == -INFINITE) bestScore = getCheckers(board) ? -CHECKMATE + sh->ply : DRAW; // TODO: Should this be considered EXACT bound?
    /*                                   */

    if (!atomic_load_explicit(&stop, memory_order_relaxed)) savePositionEvaluation(st->tt, pe, positionKey, bestMove, depth, bestScore > oldAlpha ? EXACT : UPPER, adjustNodeScoreToTT(bestScore, sh->ply));
    return bestScore;
}

static void* startSearch(void *searchThread) {
    SearchThread *st = searchThread;
    SearchHelper sh[MAX_DEPTH + 1];
    sh[0].ply = 0;
    
    char pvString[2048], bestMove[6], ponderMove[6];
    double totalTime;
    Score score;
    clock_t start = clock(); // TODO: Use real time
    for (Depth depth = 1; !atomic_load_explicit(&stop, memory_order_relaxed); depth++) {
        score = alphaBeta(-INFINITE, INFINITE, depth, sh, true, st);
        totalTime = (double) (clock() - start) / CLOCKS_PER_SEC;

        if (!atomic_load_explicit(&stop, memory_order_relaxed)) {
            pvToString(pvString, bestMove, ponderMove, sh[0].pv);
            // TODO: Should eventually include seldepth, nodes, score mate, nps, maybe others
            if (st->print) printf("info depth %d time %.0lf score cp %d pv %s\n", depth, totalTime * 1000.0, score, pvString);
        }
    }
    if (st->print) {
        if (ponderMove[0]) printf("bestmove %s ponder %s\n", bestMove, ponderMove);
        else printf("bestmove %s\n", bestMove);
    }
    return nullptr;
}

void startSearchThreads(UCI_Configuration *restrict config) {
    config->tt.age++;

    pthread_t th;
    SearchThread st;
    atomic_store_explicit(&stop, false, memory_order_relaxed);
    createSearchThread(&st, &config->board, &config->tt, true);
    pthread_create(&th, nullptr, startSearch, &st);

    time_t start = time(nullptr);
    while (time(nullptr) - start <= 1.0);

    atomic_store_explicit(&stop, true, memory_order_relaxed);
    pthread_join(th, nullptr);
}
