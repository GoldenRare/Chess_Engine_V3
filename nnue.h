#ifndef NNUE_H
#define NNUE_H

#include "utility.h"

constexpr int FLIP_MASK = 0b111000;

constexpr int SCORE_SCALE     = 400;
constexpr int QUANTIZATION_A  = 255;
constexpr int QUANTIZATION_B  =  64;

constexpr int PERSPECTIVE = 2;
constexpr int LAYER1 = 128;

typedef struct Network {
    int16_t accumulatorWeights[COLOURS][PIECE_TYPES - 1][SQUARES][LAYER1];
    int16_t accumulatorBiases[LAYER1];

    int16_t outputWeights[LAYER1 * PERSPECTIVE];
    int16_t outputBias;
} Network;

typedef struct Accumulator {
    int16_t accumulator[COLOURS][LAYER1];
} Accumulator;

void initializeNNUE();

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
