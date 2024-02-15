#ifndef SEARCH_H
#define SEARCH_H

#include "chess_board.h"

void startSearch(ChessBoard *board, int depth);
int alphaBeta(ChessBoard *board, int alpha, int beta, int depth);

#endif