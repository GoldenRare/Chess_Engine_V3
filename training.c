#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "chess_board.h"
#include "search.h"
#include "utility.h"
#include "move_generator.h"

static FILE* trainingData;

typedef struct GameData {
    struct GameData *next;
    Score score;
    char fen[128];
} GameData;

// Score parameter is relative to the side to move
static void createGameData(GameData *restrict current, const ChessBoard *restrict board, Score score) {
    GameData *gd = malloc(sizeof(GameData));
    gd->next = nullptr;
    gd->score = board->sideToMove ? -score : score;
    getFEN(board, gd->fen);
    current->next = gd;
}

// Does not destroy head itself
static void destroyGameData(GameData *restrict head) {
    GameData *destroy = head->next;
    while (destroy) {
        GameData *temp = destroy->next;
        free(destroy);
        destroy = temp;
    }
    head->next = nullptr;
}

// Does not write head itself, outcome of the game is relative to white (1.0 == white won, 0.5 == draw, 0.0 == black won)
static int writeGameData(const GameData *restrict head, double outcome) {
    int positions = 0;
    GameData *write = head->next;
    while (write) {
        positions++;
        fprintf(trainingData, "%s | %d | %.1f\n", write->fen, write->score, outcome);
        write = write->next;
    }
    return positions;
}

static inline void swap(MoveObject *mo1, MoveObject *mo2) {
    MoveObject temp = *mo1;
    *mo1 = *mo2;
    *mo2 = temp;
}

static inline bool isCheckmate(Score score) {
    return score <= CHECKMATED || score >= -CHECKMATED;
}

static inline bool isStalemate(Move bestMove) {
    return bestMove == NO_MOVE;
}

static inline bool isEndOfGame(const ChessBoard *restrict board, const MoveObject *restrict moveObj) {
    return isDraw(board) || isCheckmate(moveObj->score) || isStalemate(moveObj->move);
}

static void playRandomMoves(ChessBoard *restrict board, int numberOfRandomMoves) {
    for (int i = 0; i < numberOfRandomMoves; i++) {
        MoveObject moveList[MAX_MOVES];
        MoveObject *startList = moveList;
        MoveObject *endList = createMoveList(board, moveList, CAPTURES);
        endList = createMoveList(board, endList, NON_CAPTURES);
        size_t moveListSize = endList - startList;
        while (moveListSize) {
            MoveObject *moveObj = &startList[rand() % moveListSize];
            Move move = moveObj->move;
            if (isLegalMove(board, move)) {
                IrreversibleBoardState ibs;
                makeMove(board, move, &ibs);
                break;
            }
            moveListSize--;
            swap(startList++, moveObj);
        }
    }
}

// Randomly plays the first 5-10 moves
static void playRandomGame() {
    clock_t start = clock();
    ChessBoard board = {0};
    parseFEN(&board, START_POS);
    playRandomMoves(&board, rand() % 6 + 5);

    GameData head = {.next = nullptr};
    GameData *current = &head;
    MoveObject bestMove = {NO_MOVE + 1, 0};
    while (!isEndOfGame(&board, &bestMove)) {
        bestMove = startSearch(&board, 5);
        if (!board.checkers && !isCheckmate(bestMove.score)) {
            createGameData(current, &board, bestMove.score);
            current = current->next;
        }
        IrreversibleBoardState ibs;
        makeMove(&board, bestMove.move, &ibs);
    }
    double outcome = 0.5;
    if (isCheckmate(bestMove.score)) {
        Colour winner = bestMove.score > 0 ? !board.sideToMove : board.sideToMove;
        outcome = winner ? 0.0 : 1.0;
    }
    int positions = writeGameData(&head, outcome);
    destroyGameData(&head);
    double totalTime = (double) (clock() - start) / CLOCKS_PER_SEC;
    printf("info string positions/sec: %.0f\n", positions / totalTime);
}

void initializeTraining() {
    srand(time(nullptr));
}

void startTraining() {
    trainingData = fopen("training_data.txt", "a"); // TODO: Close the file
    playRandomGame(); // TODO: Be able to stop
}
