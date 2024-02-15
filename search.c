#include <stdio.h>
#include <time.h>
#include "search.h"
#include "chess_board.h"
#include "utility.h"
#include "move_generator.h"
#include "evaluation.h"

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

   if (depth == 0) return evaluation(board);
   
    IrreversibleBoardState ibs;
    Move moveList[256];
    Move *endList = createMoveList(board, moveList);
    Bitboard pinnedPieces = getPinnedPieces(board);
    int score = -INFINITE;
    int legalMoves = 0;
    for (Move *move = moveList; move != endList; move++) {
        if (!isLegalMove(board, move, pinnedPieces)) continue;
        legalMoves++;
        makeMove(board, move, &ibs);
        score = -alphaBeta(board, -beta, -alpha, depth - 1);
        undoMove(board, move, &ibs);

        if (score >= beta) return beta; // Fail hard
        if (score > alpha) alpha = score;
    }

    /* CHECKMATE AND STALEMATE DETECTION */
    if (!legalMoves) {
        Colour stm = board->sideToMove;
        alpha = attackersTo(board, getKingSquare(board, stm), stm, getOccupiedSquares(board)) ? CHECKMATED - depth : DRAW; // Temporary solution
    }
    /*                                   */

    return alpha;
}