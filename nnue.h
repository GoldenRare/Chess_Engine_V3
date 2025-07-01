#ifndef NNUE_H
#define NNUE_H

#include "utility.h"

void initializeNNUE();

void resetAccumulator();
// Adds a piece to the accumulator, always using the perspective of white.
void accumulatorAdd(Colour c, PieceType pt, Square sq);
// Removes a piece from the accumulator, always using the perspective of white.
void accumulatorSub(Colour c, PieceType pt, Square sq);
// Adds a piece to the accumulator, and removes it from the square it was previously on.
// Always using the perspective of white.
void accumulatorAddSub(Colour c, PieceType pt, Square fromSquare, Square toSquare);

Score evaluation(Colour stm);

#endif
