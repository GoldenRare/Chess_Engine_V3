#ifndef MOVE_GENERATOR_H
#define MOVE_GENERATOR_H

#define NUMBER_OF_SLIDING_DIRECTIONS 4
#define MAX_SLIDING_ATTACKS 107648

#include <stdint.h>
#include <stdbool.h>
#include "chess_board.h"
#include "utility.h"

typedef uint16_t Move;

inline void setFromSquare(Move *move, Square sq) {
    *move |= sq;
}

inline void setToSquare(Move *move, Square sq) {
    *move |= sq << 6;
}

extern Bitboard pieceAttacks[PIECE_TYPES][SQUARES];
extern Bitboard slidingAttacks[MAX_SLIDING_ATTACKS];

void initializeMoveGenerator();
void initializePieceAttacks();
void initializeSlidingAttacks();
Bitboard generateKnightAttacks(Bitboard knightSq);
Bitboard generateSlidingAttacks(const Direction directions[], size_t numDirections, Square sq, Bitboard occupied);
Bitboard generateKingAttacks(Bitboard kingSq);
/* Essentially checks if fromSq and toSq are within a king ring distance from each other. */
bool isDirectionMaintained(Square fromSq, Square toSq); 

#endif