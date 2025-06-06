#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uci.h"
#include "chess_board.h"
#include "move_generator.h"
#include "utility.h"
#include "search.h"
#include "transposition_table.h"

static const char *startPos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static const char *quit      = "quit";
static const char *uci       = "uci";
static const char *isready   = "isready";
static const char *benchmark = "benchmark";
static const char *position  = "position";
static const char *go        = "go";
static const char *setoption = "setoption";

static void processUCICommand() {
    puts("id name GoldenRareBOT V3");
    puts("id author Deshawn Mohan");
    // TODO: List options
    puts("uciok");
}

static void processIsReadyCommand() {
    puts("readyok");
}

static uint64_t perft(ChessBoard *restrict board, Depth depth) {
    uint64_t nodes = 0;
    IrreversibleBoardState ibs;
    MoveObject moveList[256];
    MoveObject *endList = createMoveList(board, moveList, CAPTURES);
    endList = createMoveList(board, endList, NON_CAPTURES);
    if (depth == 1)
        for (MoveObject *moveObj = moveList; moveObj != endList; moveObj++) 
            nodes += isLegalMove(board, &moveObj->move);
    else 
        for (MoveObject *moveObj = moveList; moveObj != endList; moveObj++) {
            const Move *move = &moveObj->move;
            if (!isLegalMove(board, move)) continue;
            makeMove(board, move, &ibs);
            nodes += perft(board, depth - 1);
            undoMove(board, move, &ibs);
        }
    return nodes;
}

static void runBenchmark(Depth depth) {
    FILE *perftFile = fopen("perft_test_cases.txt", "r");
    char line[256];

    double totalTime = 0.001; // TODO: Non ideal way to guard divide by 0 below
    uint64_t actualNodes = 0, expectedNodes = 0;
    while (fgets(line, sizeof(line), perftFile)) {
        ChessBoard board = {0};
        parseFEN(&board, strtok(line, ","));

        clock_t start = clock(); // TODO: Consider a more accurate clock
        actualNodes += perft(&board, depth);
        totalTime += (double) (clock() - start) / CLOCKS_PER_SEC;
        
        for (int i = 0; i < depth - 1; i++) strtok(NULL, ",");
        expectedNodes += strtoull(strtok(NULL, ","), NULL, 10);
    }

    printf("info string BENCHMARK %s\n", actualNodes == expectedNodes ? "PASSED" : "FAILED");
    printf("info depth %d time %.0lf nodes %llu nps %.0lf\n", depth, totalTime * 1000.0, actualNodes, actualNodes / totalTime);
    fclose(perftFile);
}

void uciLoop() {
    ChessBoard board;
    char input[512]; // TODO: Figure out max size
    char *token = "";

    while (!token || strcmp(token, quit)) {
        fgets(input, sizeof(input), stdin);
        size_t length = strlen(input);
        if (length && input[length - 1] == '\n') input[length - 1] = '\0';

        token = strtok(input, " ");
        if (token == NULL) continue;

        if      (strcmp(token, uci)       == 0) processUCICommand();
        else if (strcmp(token, isready)   == 0) processIsReadyCommand();
        else if (strcmp(token, benchmark) == 0) processBenchmarkCommand();
        else if (strcmp(token, position)  == 0) processPositionCommand(&board);
        else if (strcmp(token, go)        == 0) processGoCommand(&board);
        else if (strcmp(token, setoption) == 0) processSetOptionCommand();

        char *tokenHelper = token;
        while (tokenHelper != NULL) tokenHelper = strtok(NULL, " ");
    }
    
}

void processBenchmarkCommand() {
    char *depth = strtok(NULL, " ");
    runBenchmark(strtol(depth, NULL, 10));
}

void processPositionCommand(ChessBoard *board) {
    static const char *fen = "fen";
    static const char *moves = "moves";
    const char *pos;
    char positionToSet[128] = "";
    char *token = strtok(NULL, " ");

    memset(board, 0, sizeof(ChessBoard)); // Should consider whether or not this should be handled by parseFEN
    if (strcmp(token, fen) == 0) {
        token = strtok(NULL, " ");
        while (token != NULL && strcmp(token, moves) != 0) {
            strcat(positionToSet, token); // Consider different concat operation
            strcat(positionToSet, " ");
            token = strtok(NULL, " ");
        }
        pos = positionToSet;
    } else {
        pos = startPos;
        token = strtok(NULL, " ");
    }
    parseFEN(board, pos);
    if (token != NULL && strcmp(token, moves) == 0) processMoves(board);
}

//position startpos moves e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f8e7 d2d3 b7b5 a4b3 d7d6 c2c3 e8g8 b1d2 c8g4 h2h3 g4h5 a2a4 b5b4 c3c4 f6d7 b3c2 f7f5
//position fen 7k/5P2/8/8/8/8/8/7K w - - 0 1 moves f7f8q h8h7 f8b4 h7g7 h1g2 g7f6 b4d2
//position fen 4q2k/5P2/8/8/8/8/8/7K w - - 0 1 moves f7e8q h8g7
void processMoves(ChessBoard *board) {
    char *moveStr;
    while ((moveStr = strtok(NULL, " ")) != NULL) {
        MoveObject moveList[MAX_MOVES];
        MoveObject *endList = createMoveList(board, moveList, CAPTURES); // TODO
        char moveToName[6] = "";
        for (MoveObject *startList = moveList; startList < endList; startList++) {
            MoveType moveType = getMoveType(startList->move);
            char promotionPiece = moveType & PROMOTION ? PROMOTION_NAME[moveType & PROMOTION_PIECE_OFFSET_MASK] : '\0';
            encodeChessMove(moveToName, SQUARE_NAME[getFromSquare(startList->move)], SQUARE_NAME[getToSquare(startList->move)], promotionPiece);
            if (strcmp(moveStr, moveToName) == 0) {
                IrreversibleBoardState ibs;
                makeMove(board, &startList->move, &ibs);
                break;
            }
        }
    }
}

void processGoCommand(ChessBoard *board) {
    static const char *depth = "depth";
    int searchDepth = MAX_DEPTH;
    
    char *token = strtok(NULL, " ");
    if (token != NULL && strcmp(depth, token) == 0) searchDepth = strtol(strtok(NULL, " "), NULL, 10);

    startSearch(board, searchDepth);
}

void processSetOptionCommand() {
    static const char *Hash = "Hash";
    
    strtok(NULL, " "); // Discard since this should just be "name"
    char *token = strtok(NULL, " ");

    if (strcmp(token, Hash) == 0) processHashOption();

}

void processHashOption() {
    strtok(NULL, " "); // Discard value string
    setTranspositionTableSize(strtol(strtok(NULL, " "), NULL, 10));
}