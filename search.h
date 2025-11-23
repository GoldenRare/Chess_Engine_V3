#ifndef SEARCH_H
#define SEARCH_H

#include <stdint.h>
#include <time.h>
#include "chess_board.h"
#include "transposition_table.h"
#include "uci.h"

typedef struct SearchThread {
    ChessBoard board;
    TT *tt;
    uint64_t startNs; // TODO: Could change implementation
    uint64_t maxSearchTimeNs;
    bool print;
} SearchThread;

static inline uint64_t getTimeNs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t) ts.tv_sec * 1000000000ULL + (uint64_t) ts.tv_nsec;
}

static inline void createSearchThread(SearchThread *st, const ChessBoard *restrict board, TT *tt, uint64_t startNs, uint64_t maxSearchTimeNs, bool print) {
    st->board = *board; // TODO: The history pointer is a shallow copy, consider using a deep copy
    st->tt = tt;
    st->startNs = startNs;
    st->maxSearchTimeNs = maxSearchTimeNs;
    st->print = print;
}

static inline bool outOfTime(const SearchThread *st) {
    return getTimeNs() - st->startNs >= st->maxSearchTimeNs;
}

void startSearchThreads(UCI_Configuration *restrict config, uint64_t searchTimeNs);

#endif
