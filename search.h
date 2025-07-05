#ifndef SEARCH_H
#define SEARCH_H

#include "chess_board.h"
#include "transposition_table.h"
#include "utility.h"

typedef struct SearchThread {
    TT *tt;
} SearchThread;

constexpr Depth MAX_DEPTH = 8;

MoveObject startSearch(ChessBoard *restrict board, Depth depth, SearchThread *st);

#endif
