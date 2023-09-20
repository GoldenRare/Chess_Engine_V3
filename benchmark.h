#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdint.h>
#include "chess_board.h"

void runBenchmark();
uint64_t perft(const ChessBoard *board, int depth);

#endif