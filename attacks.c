#include "attacks.h"
#include "utility.h"

Bitboard pawnAttacks[COLOURS][SQUARES];
Bitboard nonSliderAttacks[NON_SLIDERS][SQUARES]; // Does not include pawns
SliderAttacks sliderAttacks[SLIDERS][SQUARES]; // Does not include the queen
static Bitboard slidingAttacks[107648]; // 107648 - Represents all the number of attacks for Bishop and Rook depending on the square and ray occupancy 

static inline Bitboard initializeKnightAttacks(Bitboard knightSq) {
    return shiftBitboard(shiftBitboard(knightSq &              ~FILE_H_BB, NORTH), NORTH_EAST) 
         | shiftBitboard(shiftBitboard(knightSq & ~FILE_G_BB & ~FILE_H_BB, EAST),  NORTH_EAST)
         | shiftBitboard(shiftBitboard(knightSq & ~FILE_G_BB & ~FILE_H_BB, EAST),  SOUTH_EAST)
         | shiftBitboard(shiftBitboard(knightSq &              ~FILE_H_BB, SOUTH), SOUTH_EAST)
         | shiftBitboard(shiftBitboard(knightSq &              ~FILE_A_BB, SOUTH), SOUTH_WEST)
         | shiftBitboard(shiftBitboard(knightSq & ~FILE_A_BB & ~FILE_B_BB, WEST),  SOUTH_WEST)
         | shiftBitboard(shiftBitboard(knightSq & ~FILE_A_BB & ~FILE_B_BB, WEST),  NORTH_WEST)
         | shiftBitboard(shiftBitboard(knightSq &              ~FILE_A_BB, NORTH), NORTH_WEST);
}

static inline Bitboard initializeKingAttacks(Bitboard kingSq) {
    Bitboard attacks = shiftBitboard(kingSq & ~FILE_H_BB, EAST) | shiftBitboard(kingSq & ~FILE_A_BB, WEST);
    kingSq |= attacks;
    return attacks | shiftBitboard(kingSq, NORTH) | shiftBitboard(kingSq, SOUTH);
}

static Bitboard initializeSlidingAttacks(const Direction *restrict directions, int numDirections, Square sq, Bitboard occupied) {
    Bitboard attacks = 0;
    for (int i = 0; i < numDirections; i++) {
        Square fromSq = sq;
        Square toSq = moveSquareInDirection(sq, directions[i]);
        Bitboard toSquareBB = shiftBitboard(squareToBitboard(sq), directions[i]);
        while (toSquareBB && isAdjacentSquare(fromSq, toSq)) {
            attacks |= toSquareBB;
            if (toSquareBB & occupied) break;
            toSquareBB = shiftBitboard(toSquareBB, directions[i]);
            fromSq = toSq;
            toSq = moveSquareInDirection(fromSq, directions[i]);
        }
    }
    return attacks;
}

static void initializeNonSliderAttacks() {
    for (Square sq = 0; sq < SQUARES; sq++) {
        Bitboard sqBB = squareToBitboard(sq);
        pawnAttacks[WHITE][sq] = shiftBitboard(sqBB & ~FILE_H_BB, NORTH_EAST) | shiftBitboard(sqBB & ~FILE_A_BB, NORTH_WEST);
        pawnAttacks[BLACK][sq] = shiftBitboard(sqBB & ~FILE_H_BB, SOUTH_EAST) | shiftBitboard(sqBB & ~FILE_A_BB, SOUTH_WEST);
        nonSliderAttacks[KNIGHT_NON_SLIDER][sq] = initializeKnightAttacks(sqBB);
        nonSliderAttacks[KING_NON_SLIDER][sq] = initializeKingAttacks(sqBB);
    }
}

static void initializeSliderAttacks() {
    const Direction directions[SLIDERS][4] = {{NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST}, {NORTH, SOUTH, EAST, WEST}};
    int count = 0;

    for (Slider slider = BISHOP_SLIDER; slider < SLIDERS; slider++) {
        for (Square sq = 0; sq < SQUARES; sq++) {
            Bitboard edges = ((RANK_1_BB | RANK_8_BB) & ~rankBitboardOfSquare(sq)) | ((FILE_A_BB | FILE_H_BB) & ~fileBitboardOfSquare(sq));
            Bitboard relevantOccupancyMask = initializeSlidingAttacks(directions[slider], 4, sq, 0) & ~edges;
            
            int startIndex = count;
            Bitboard occupiedSubset = 0;
            sliderAttacks[slider][sq].mask = relevantOccupancyMask;
            sliderAttacks[slider][sq].attacks = &slidingAttacks[startIndex];
            do {
                Bitboard attacks = initializeSlidingAttacks(directions[slider], 4, sq, occupiedSubset);
                slidingAttacks[startIndex + pext(occupiedSubset, relevantOccupancyMask)] = attacks;
                occupiedSubset = (occupiedSubset - relevantOccupancyMask) & relevantOccupancyMask;
                count++;
            } while (occupiedSubset);
        }
    }
}

void initializeAttacks() {
    initializeNonSliderAttacks();
    initializeSliderAttacks();
}
