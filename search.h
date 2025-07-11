#ifndef SEARCH_H
#define SEARCH_H

#include <stddef.h>
#include "chess_board.h"
#include "transposition_table.h"
#include "utility.h"

constexpr Depth MAX_DEPTH = 8;

typedef struct SearchThread {
    TT tt;
} SearchThread;

static inline void createSearchThread(SearchThread *restrict st, size_t MB) {
    createTranspositionTable(&st->tt, MB);
}

MoveObject startSearch(ChessBoard *restrict board, Depth depth, SearchThread *st);

#endif
