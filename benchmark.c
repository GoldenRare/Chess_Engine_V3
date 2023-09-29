#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include "benchmark.h"
#include "chess_board.h"
#include "move_generator.h"
#include "utility.h"

void runBenchmark(int depth) {
    FILE *perftFile = fopen("perft_test_cases.txt", "r");
    char *token = "";
    char line[256];

    const int MAX_DEPTH = 6;
    int correct = 0;
    int totalPositions = 0;
    depth = depth > MAX_DEPTH ? MAX_DEPTH : depth;
    while (fgets(line, sizeof(line), perftFile)) {
        totalPositions++;
        token = strtok(line, ",");
        ChessBoard board = {0};
        parseFEN(&board, token);

        for (size_t i = 0; i < depth; i++) token = strtok(NULL, ",");
        uint64_t expectedNodes = strtoull(token, NULL, 10);

        // Should consider switching clock() to something more precise since measurements are essentially discrete
        clock_t start = clock();
        uint64_t nodes = perft(&board, depth);
        clock_t end = clock();
        double time = (double) (end - start) / CLOCKS_PER_SEC;
        double nps = time > 0.0 ? nodes / time : nodes / 0.001;
        
        if (expectedNodes == nodes) correct++;
        printf("info depth %d time %.0lf nodes %" PRIu64 " nps %.0lf\n", depth, time * 1000.0, nodes, nps);
    }
    printf("Result: %d/%d\n", correct, totalPositions);
    fclose(perftFile);
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