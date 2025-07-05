#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "training.h"
#include "chess_board.h"
#include "search.h"
#include "utility.h"
#include "move_generator.h"

typedef struct GameData {
    struct GameData *prev;
    Score score;
    char fen[128];
} GameData;

// Score parameter is relative to the side to move
static void createGameData(GameData *current, GameData *previous, const ChessBoard *restrict board, Score score) {
    current->prev  = previous;
    current->score = board->sideToMove ? -score : score;
    getFEN(board, current->fen);
}

// Outcome of the game is relative to white (1.0 == white won, 0.5 == draw, 0.0 == black won)
static int writeGameData(const GameData *restrict data, FILE *restrict file, double outcome) {
    int positions = 0;
    // A dummy node is placed at the end, this node can be found by checking if it leads to null in prev
    while (data->prev) {
        positions++;
        fprintf(file, "%s | %d | %.1f\n", data->fen, data->score, outcome);
        data = data->prev;
    }
    return positions;
}

static inline void swap(MoveObject *mo1, MoveObject *mo2) {
    MoveObject temp = *mo1;
    *mo1 = *mo2;
    *mo2 = temp;
}

static inline bool isCheckmate(Score score) {
    return score <= -GUARANTEE_CHECKMATE || score >= GUARANTEE_CHECKMATE;
}

static inline bool isStalemate(Score score, Move bestMove) {
    return score == DRAW && !bestMove;
}

static inline bool isEndOfGame(const ChessBoard *restrict board, const MoveObject *restrict moveObj) {
    return isDraw(board) || isStalemate(moveObj->score, moveObj->move) || isCheckmate(moveObj->score);
}

static void playRandomMoves(ChessBoard *board, ChessBoardHistory *history, int numberOfRandomMoves) {
    for (int i = 0; i < numberOfRandomMoves; i++) {
        MoveObject moveList[MAX_MOVES];
        MoveObject *startList = moveList;
        MoveObject *endList = createMoveList(board, moveList, CAPTURES);
        endList = createMoveList(board, endList, NON_CAPTURES);
        size_t moveListSize = endList - startList;
        while (moveListSize) {
            MoveObject *moveObj = &startList[rand() % moveListSize];// TODO: RAND
            Move move = moveObj->move;
            if (isLegalMove(board, move)) {
                makeMove(board, &history[i], move);
                break;
            }
            moveListSize--;
            swap(startList++, moveObj);
        }
    }
}

static void playGame(ChessBoard *restrict board, GameData *restrict previous, FILE *restrict file) {
    ChessBoardHistory history;
    GameData current;
    MoveObject bestMove = startSearch(board, 5);
    if (!getCheckers(board) && !isCheckmate(bestMove.score)) { // TODO: What positions to save?
        createGameData(&current, previous, board, bestMove.score);
        previous = &current;
    }
    if (isEndOfGame(board, &bestMove)) {
        double outcome = 0.5;
        if (isCheckmate(bestMove.score)) {
            Colour winner = bestMove.score > 0 ? board->sideToMove : !board->sideToMove;
            outcome = winner ? 0.0 : 1.0;
        }
        writeGameData(previous, file, outcome);
        return;
    }
    makeMove(board, &history, bestMove.move);
    playGame(board, previous, file);
}

// Randomly plays the first 5-10 moves
static void playRandomGame(FILE *restrict file) {
    constexpr int MAX_RANDOM_MOVES = 10;
    ChessBoard board = {0};
    ChessBoardHistory history[MAX_RANDOM_MOVES + 1] = {0};
    GameData dummy = {.prev = nullptr};
    parseFEN(&board, history, START_POS); // TODO: Accumulator
    playRandomMoves(&board, &history[1], rand() % 6 + 5); // TODO: RAND
    playGame(&board, &dummy, file); // TODO: Is it safe to write data for position that randomly is draw?
}

void* startTraining(void *trainingThread) {
    TrainingThread *tt = trainingThread;
    while (!tt->stop) playRandomGame(tt->file);
    fclose(tt->file); // TODO
    return nullptr;
}
