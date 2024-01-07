#ifndef ZOBRIST_H
#define ZOBRISH_H

#include <stdint.h>
#include "utility.h"

typedef struct Zobrist { 
    uint64_t pieceOnSquare[ALL_PIECES * COLOURS][SQUARES];
    uint64_t castlingRights[ALL_RIGHTS + 1]; // +1 to include ALL_RIGHTS
    uint64_t enPassant[FILES];
    uint64_t sideToMove;
} Zobrist;

extern Zobrist zobristHashes;

// Random number generator derived from Stockfish, more precisely from Sebastiano Vigna (2014).
inline uint64_t random64BitNumber(uint64_t *seed) {
    *seed ^= *seed >> 12, *seed ^= *seed << 25, *seed ^= *seed >> 27;
    return *seed * 2685821657736338717LL;
}

void initializeZobrist();

#endif