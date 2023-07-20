#include <stdlib.h>
#include "move_generator.h"
#include "utility.h"

Bitboard knightAttacks[SQUARES];

void initializeMoveGenerator() {
    initializeKnightAttacks();
}

void initializeKnightAttacks() {
    for (Square sq = 0; sq < SQUARES; sq++) {
        Bitboard b = squareToBitboard(sq);
        knightAttacks[sq] = shiftBitboard(shiftBitboard(b &              ~FILE_H_BB, NORTH), NORTH_EAST) 
                          | shiftBitboard(shiftBitboard(b & ~FILE_G_BB & ~FILE_H_BB, EAST),  NORTH_EAST)
                          | shiftBitboard(shiftBitboard(b & ~FILE_G_BB & ~FILE_H_BB, EAST),  SOUTH_EAST)
                          | shiftBitboard(shiftBitboard(b &              ~FILE_H_BB, SOUTH), SOUTH_EAST)
                          | shiftBitboard(shiftBitboard(b &              ~FILE_A_BB, SOUTH), SOUTH_WEST)
                          | shiftBitboard(shiftBitboard(b & ~FILE_A_BB & ~FILE_B_BB, WEST),  SOUTH_WEST)
                          | shiftBitboard(shiftBitboard(b & ~FILE_A_BB & ~FILE_B_BB, WEST),  NORTH_WEST)
                          | shiftBitboard(shiftBitboard(b &              ~FILE_A_BB, NORTH), NORTH_WEST);
    }
}

Bitboard generateSlidingAttacks(const Direction directions[], size_t numDirections, Square sq, Bitboard occupied) {
    Bitboard attacks = 0ULL;
    for (size_t i = 0; i < numDirections; i++) {
        Square fromSq = sq;
        Square toSq = toSquare(sq, directions[i]);
        Bitboard toSquareBB = shiftBitboard(squareToBitboard(sq), directions[i]);
        while (toSquareBB && isDirectionMaintained(fromSq, toSq)) {
            attacks |= toSquareBB;
            if (toSquareBB & occupied) break;
            toSquareBB = shiftBitboard(toSquareBB, directions[i]);
            fromSq = toSq;
            toSq = toSquare(fromSq, directions[i]);
        }
    }
    return attacks;
}

bool isDirectionMaintained(Square fromSq, Square toSq) {
    int rankDistance = abs(squareToRank(toSq) - squareToRank(fromSq));
    int fileDistance = abs(squareToFile(toSq) - squareToFile(fromSq));
    return max(rankDistance, fileDistance) == 1;
}