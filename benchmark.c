#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <inttypes.h>
#include "benchmark.h"
#include "chess_board.h"
#include "move_generator.h"
#include "utility.h"

void runBenchmark() {
    int depth = 6;
    ChessBoard board = {0};
    parseFEN(&board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    clock_t start = clock();
    uint64_t nodes = perft(&board, depth);
    clock_t end = clock();
    double time = (double) (end - start) / CLOCKS_PER_SEC; 
    double nps = nodes / time; // Should maybe check for / 0
    printf("info depth %d time %.0lf nodes %" PRIu64 " nps %.0lf\n", depth, time * 1000.0, nodes, nps);
}

uint64_t perft(const ChessBoard *board, int depth) {
    if (depth == 0) return 1ULL;

    uint64_t nodes = 0;
    Move moveList[256];
    Move *startList = moveList;
    Move *endList = generateAllMoves(board, startList);
    while (startList < endList) {
        ChessBoard copyBoard = *board;
        makeMove(&copyBoard, startList);
        if (!isSquareAttacked(&copyBoard, bitboardToSquare(getPieces(&copyBoard, board->sideToMove, KING)), board->sideToMove)) {
            nodes += perft(&copyBoard, depth - 1);
        }
        startList++;
    }

    return nodes;
}