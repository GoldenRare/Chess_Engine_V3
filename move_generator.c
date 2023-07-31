#include <stdlib.h>
#include "move_generator.h"
#include "utility.h"

Bitboard knightAttacks[SQUARES];
Bitboard slidingAttacks[MAX_SLIDING_ATTACKS];

void initializeMoveGenerator() {
    initializeKnightAttacks();
    initializeSlidingAttacks();
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

void initializeSlidingAttacks() {
    enum {numSlidingGroups = 2};
    const Direction slidingDirections[numSlidingGroups][NUMBER_OF_SLIDING_DIRECTIONS] = {{NORTH, SOUTH, EAST, WEST}, {NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST}};
    Bitboard count = 0;

    for (size_t i = 0; i < numSlidingGroups; i++) {
        for (Square sq = 0; sq < SQUARES; sq++) {
            Bitboard edges = ((RANK_1_BB | RANK_8_BB) & ~rankBitboardOfSquare(sq)) | ((FILE_A_BB | FILE_H_BB) & ~fileBitboardOfSquare(sq));
            Bitboard relevantOccupancyMask = generateSlidingAttacks(slidingDirections[i], NUMBER_OF_SLIDING_DIRECTIONS, sq, 0) & ~edges;
                                  
            Bitboard occupiedSubset = 0;
            Bitboard startIndex = count;
            do {
                Bitboard attacks = generateSlidingAttacks(slidingDirections[i], NUMBER_OF_SLIDING_DIRECTIONS, sq, occupiedSubset);
                slidingAttacks[startIndex + pext(occupiedSubset, relevantOccupancyMask)] = attacks;
                occupiedSubset = (occupiedSubset - relevantOccupancyMask) & relevantOccupancyMask;
                count++;
            } while (occupiedSubset);
        }
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