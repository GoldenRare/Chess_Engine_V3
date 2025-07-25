#ifndef SEARCH_H
#define SEARCH_H

#include <stddef.h>
#include "chess_board.h"
#include "transposition_table.h"
#include "utility.h"

constexpr Depth MAX_DEPTH = 7;

typedef struct SearchThread {
    TT tt;
    bool print;
} SearchThread;

// When creating a new search thread, ensure that a call to destroySearchThread() is made when the thread is no longer needed
static inline void createSearchThread(SearchThread *restrict st, size_t hashSize, bool print) {
    createTranspositionTable(&st->tt, hashSize);
    st->print = print;
}

// Destroys a previously created search thread, ensure that a call to createSearchThread() is made before using this function
static inline void destroySearchThread(SearchThread *st) {
    destroyTranspositionTable(&st->tt);
}

MoveObject startSearch(ChessBoard *restrict board, Depth depth, SearchThread *st);

#endif
