#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uci.h"
#include "benchmark.h"
#include "chess_board.h"
#include "move_generator.h"
#include "utility.h"
#include "search.h"

static const char *startPos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static const char *quit      = "quit";
static const char *uci       = "uci";
static const char *isready   = "isready";
static const char *benchmark = "benchmark";
static const char *position  = "position";
static const char *go        = "go";

void uciLoop() {
    char input[256]; // TODO: Figure out max size
    char *token = "";

    while (!token || strcmp(token, quit)) {
        ChessBoard board = {0};
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

        char *tokenHelper = token;
        while (tokenHelper != NULL) tokenHelper = strtok(NULL, " ");
    }
    
}

void processUCICommand() {
    printf("id name GoldenRareBOT V3\n");
    printf("id author Deshawn Mohan-Smith\n");
    printf("uciok\n");
}

void processIsReadyCommand() {
    printf("readyok\n");
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
    static const char *SQUARE_NAME[] = {
        "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8", 
        "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", 
        "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", 
        "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", 
        "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", 
        "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", 
        "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", 
        "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"
    };
    static const char *PROMOTION_NAME[] = {"n", "b", "r", "q"};
    char *moveStr;
    while ((moveStr = strtok(NULL, " ")) != NULL) {
        Move moveList[256];
        Move *move;
        char moveToName[6] = "";
        createMoveList(board, moveList);
        for (move = moveList; strcmp(moveStr, moveToName) != 0; move++) {
            moveToName[0] = '\0';
            strcat(moveToName, SQUARE_NAME[getFromSquare(move)]); // Consider a different concat operation like snprinf()
            strcat(moveToName, SQUARE_NAME[getToSquare  (move)]);
            MoveType moveType = getMoveType(move);
            if (moveType & PROMOTION) strcat(moveToName, PROMOTION_NAME[moveType & PROMOTION_PIECE_OFFSET_MASK]);
        }
        IrreversibleBoardState ibs;
        makeMove(board, --move, &ibs);
    }
}

void processGoCommand(ChessBoard *board) {
    static const char *depth = "depth";
    int searchDepth = MAX_DEPTH;
    
    char *token = strtok(NULL, " ");
    if (token != NULL && strcmp(depth, token) == 0) searchDepth = strtol(strtok(NULL, " "), NULL, 10);

    startSearch(board, searchDepth);
}