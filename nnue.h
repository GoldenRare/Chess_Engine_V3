#ifndef NNUE_H
#define NNUE_H

#include <stdint.h>
#include "utility.h"

constexpr int LAYER1 = 128;

typedef struct Accumulator {
    int16_t accumulator[COLOURS][LAYER1];
} Accumulator;

void accumulatorReset(Accumulator *restrict accumulator);
// Adds a piece to the accumulator, always using the perspective of white.
void accumulatorAdd(Accumulator *restrict accumulator, Colour c, PieceType pt, Square sq);
// Removes a piece from the accumulator, always using the perspective of white.
void accumulatorSub(Accumulator *restrict accumulator, Colour c, PieceType pt, Square sq);
// Adds a piece to the accumulator, and removes it from the square it was previously on.
// Always using the perspective of white.
void accumulatorAddSub(Accumulator *restrict accumulator, Colour c, PieceType pt, Square fromSquare, Square toSquare);

Score evaluation(const Accumulator *restrict accumulator, Colour stm);

#endif
