#ifndef SEARCH_H
#define SEARCH_H

#define MAX_DEPTH 7

#include <stdbool.h>
#include "chess_board.h"
#include "utility.h"

typedef struct SearchHelper {
    Move* pv; // The Principal Variation
} SearchHelper;

void startSearch(ChessBoard *board, int depth);
int alphaBeta(ChessBoard *board, int alpha, int beta, int depth, SearchHelper *sh, bool isRootNode);
int quiescenceSearch(ChessBoard *board, int alpha, int beta);
void encodePrincipalVariation(char* buffer, const Move *pv);

inline void updatePrincipalVariation(Move m, Move *currentPv, const Move *childrenPv) {
    *currentPv++ = m;
    while((*currentPv++ = *childrenPv++) != NO_MOVE);
} 

#endif