#ifndef NNUE_H
#define NNUE_H

#include "utility.h"

void initializeNNUE();

// Adds a piece to the accumulator, always using the perspective of white.
void accumulatorAdd(Colour c, PieceType pt, Square sq);

Score evaluation(Colour stm);

#endif
