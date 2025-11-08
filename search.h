#ifndef SEARCH_H
#define SEARCH_H

#include "chess_board.h"
#include "transposition_table.h"
#include "uci.h"

typedef struct SearchThread {
    ChessBoard board;
    TT *tt;
    bool print;
} SearchThread;

static inline void createSearchThread(SearchThread *st, const ChessBoard *restrict board, TT *tt, bool print) {
    st->board = *board; // TODO: The history pointer is a shallow copy, consider using a deep copy
    st->tt = tt;
    st->print = print;
}

void startSearchThreads(UCI_Configuration *restrict config, uint64_t searchTimeNs);

#endif
