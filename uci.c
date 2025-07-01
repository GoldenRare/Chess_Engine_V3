#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "chess_board.h"
#include "move_generator.h"
#include "utility.h"
#include "search.h"
#include "transposition_table.h"
#include "training.h"

// Official UCI Commands
constexpr char GO        [] = "go"       ;
constexpr char IS_READY  [] = "isready"  ;
constexpr char POSITION  [] = "position" ;
constexpr char QUIT      [] = "quit"     ;
constexpr char SET_OPTION[] = "setoption";
constexpr char UCI       [] = "uci"      ;

// Unofficial UCI Commands
constexpr char BENCHMARK[] = "benchmark";
constexpr char PERFT    [] = "perft"    ;
constexpr char TRAIN    [] = "train"    ;

static void go(ChessBoard *restrict board) {
    constexpr char depth[] = "depth";
    // TODO: Options to potentially implement. All times are in msec
    constexpr char wtime   [] = "wtime"   ;
    constexpr char btime   [] = "btime"   ;
    constexpr char winc    [] = "winc"    ;
    constexpr char binc    [] = "binc"    ;
    constexpr char nodes   [] = "nodes"   ;
    constexpr char movetime[] = "movetime";
    constexpr char infinite[] = "infinite";

    Depth searchDepth = MAX_DEPTH;
    char *token;
    while ((token = strtok(nullptr, " ")))
        if      (strcmp(token, depth   ) == 0) searchDepth = strtoul(strtok(nullptr, " "), nullptr, 10);
        else if (strcmp(token, wtime   ) == 0);
        else if (strcmp(token, btime   ) == 0);
        else if (strcmp(token, winc    ) == 0);
        else if (strcmp(token, binc    ) == 0);
        else if (strcmp(token, nodes   ) == 0);
        else if (strcmp(token, movetime) == 0);
        else if (strcmp(token, infinite) == 0)
        ;
    startSearch(board, searchDepth);
}

static void isReady() {
    puts("readyok");
}

//position startpos moves e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f8e7 d2d3 b7b5 a4b3 d7d6 c2c3 e8g8 b1d2 c8g4 h2h3 g4h5 a2a4 b5b4 c3c4 f6d7 b3c2 f7f5
//position fen 7k/5P2/8/8/8/8/8/7K w - - 0 1 moves f7f8q h8h7 f8b4 h7g7 h1g2 g7f6 b4d2
//position fen 4q2k/5P2/8/8/8/8/8/7K w - - 0 1 moves f7e8q h8g7
static void position(ChessBoard *restrict board) {
    constexpr char fen  [] = "fen";
    constexpr char moves[] = "moves";

    char *token = strtok(nullptr, " ");
    char *fenStart = nullptr;
    if (strcmp(token, fen) == 0) {
        fenStart = token = token + 4;
        while (*++token) if (*token == 'm') *--token = '\0';
    }
    ChessBoardHistory *history = board->history;
    clearBoard(board);
    parseFEN(board, history, fenStart ? fenStart : START_POS);
    //if (token != NULL && strcmp(token, moves) == 0) processMoves(board);
}

static void setOption() {
    constexpr char Hash[] = "Hash";
    
    strtok(nullptr, " "); // Discard name string
    char *token = strtok(nullptr, " ");
    strtok(nullptr, " "); // Discard value string

    if (strcmp(token, Hash) == 0) setTranspositionTableSize(strtoull(strtok(nullptr, " "), nullptr, 10));
}

static void uci() {
    puts("id name GoldenRareBOT V3");
    puts("id author Deshawn Mohan");
    puts("option name Hash type spin default 256 min x max y"); // TODO
    puts("uciok");
}

static uint64_t perft(ChessBoard *restrict board, Depth depth) {
    uint64_t nodes = 0;
    ChessBoardHistory history;
    MoveObject moveList[256];
    MoveObject *endList = createMoveList(board, moveList, CAPTURES);
    endList = createMoveList(board, endList, NON_CAPTURES);
    if (depth == 1)
        for (MoveObject *moveObj = moveList; moveObj != endList; moveObj++) 
            nodes += isLegalMove(board, moveObj->move);
    else 
        for (MoveObject *moveObj = moveList; moveObj != endList; moveObj++) {
            Move move = moveObj->move;
            if (!isLegalMove(board, move)) continue;
            makeMove(board, &history, move);
            nodes += perft(board, depth - 1);
            undoMove(board, move);
        }
    return nodes;
}

static void benchmark() {
    Depth depth = strtoul(strtok(nullptr, " "), nullptr, 10);
    FILE *perftFile = fopen("perft_test_cases.txt", "r");
    char line[256];

    double totalTime = 0.001; // TODO: Non ideal way to guard divide by 0 below
    uint64_t actualNodes = 0, expectedNodes = 0;
    while (fgets(line, sizeof(line), perftFile)) {
        ChessBoard board = {0};
        ChessBoardHistory history = {0};
        parseFEN(&board, &history, strtok(line, ","));

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

static void train() {
    startTraining();
}

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
                ChessBoardHistory history;
                makeMove(board, &history, startList->move);
                break;
            }
        }
    }
}

void uciLoop() {
    ChessBoard board = {0};
    ChessBoardHistory history = {0};
    parseFEN(&board, &history, START_POS);
    char input[512]; // Assumes input is large enough to hold '\n' from stdin
    char *token = nullptr;

    while (!token || strcmp(token, QUIT) != 0) {
        fgets(input, sizeof(input), stdin);
        input[strlen(input) - 1] = '\0';

        token = strtok(input, " ");
        if (!token) continue;

        // Official UCI Commands
        if      (strcmp(token, GO        ) == 0) go(&board);
        else if (strcmp(token, IS_READY  ) == 0) isReady();
        else if (strcmp(token, POSITION  ) == 0) position(&board);
        else if (strcmp(token, SET_OPTION) == 0) setOption();
        else if (strcmp(token, UCI       ) == 0) uci();

        // Unofficial UCI Commands
        else if (strcmp(token, BENCHMARK) == 0) benchmark();
        else if (strcmp(token, TRAIN)     == 0) train();
    }
}
