#ifndef MOVE_GENERATOR_H
#define MOVE_GENERATOR_H

#define NUMBER_OF_SLIDING_DIRECTIONS 4
#define MAX_SLIDING_ATTACKS 107648

#include <stdbool.h>
#include <stdint.h>
#include "chess_board.h"
#include "utility.h"

typedef struct Magic {
    Bitboard mask;
    uint32_t offset;
} Magic;

enum MagicIndex {
    BISHOP_INDEX, ROOK_INDEX, MAGIC_INDICES
};
typedef enum MagicIndex MagicIndex;

extern Magic magicTable[MAGIC_INDICES][SQUARES];
extern Bitboard pawnAttacks[COLOURS][SQUARES];
extern Bitboard pieceAttacks[PIECE_ATTACKS_SIZE][SQUARES];
extern Bitboard slidingAttacks[MAX_SLIDING_ATTACKS];

void initializeMoveGenerator();
void initializeNonSlidingAttacks();
void initializeSlidingAttacks();

Bitboard generateKnightAttacks(Bitboard knightSq);
Bitboard generateSlidingAttacks(const Direction directions[], size_t numDirections, Square sq, Bitboard occupied);
Bitboard generateKingAttacks(Bitboard kingSq);

void generateKnightMoves(const ChessBoard *board, Move *moveList);

/* Essentially checks if fromSq and toSq are within a king ring distance from each other. */
bool isDirectionMaintained(Square fromSq, Square toSq); 

#endif