#include <stdint.h>
#include <stdio.h>
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
static Network network; // TODO: Can we make it const?

typedef struct Accumulator {
    int16_t accumulator[COLOURS][LAYER1];
} Accumulator;
static Accumulator accumulator;

static inline int32_t SCReLU(int16_t val) {
    return val >= QUANTIZATION_A ? QUANTIZATION_A * QUANTIZATION_A 
         : val >  0              ? val * val
         : 0; 
}

void initializeNNUE() {
    FILE *file = fopen("temp.bin", "rb");
    fread(&network, sizeof(Network), 1 , file);
    fclose(file);
}

void resetAccumulator() {
    for (int i = 0; i < LAYER1; i++) {
        accumulator.accumulator[WHITE][i] = network.accumulatorBiases[i];
        accumulator.accumulator[BLACK][i] = network.accumulatorBiases[i];
    }
}

void accumulatorAdd(Colour c, PieceType pt, Square sq) {
    pt--;
    for (int i = 0; i < LAYER1; i++) {
        accumulator.accumulator[WHITE][i] += network.accumulatorWeights[c][pt][sq ^ FLIP_MASK][i];
    }

    for (int i = 0; i < LAYER1; i++) {
        accumulator.accumulator[BLACK][i] += network.accumulatorWeights[c ^ 1][pt][sq][i];
    }
}

void accumulatorSub(Colour c, PieceType pt, Square sq) {
    pt--;
    for (int i = 0; i < LAYER1; i++) {
        accumulator.accumulator[WHITE][i] -= network.accumulatorWeights[c][pt][sq ^ FLIP_MASK][i];
    }

    for (int i = 0; i < LAYER1; i++) {
        accumulator.accumulator[BLACK][i] -= network.accumulatorWeights[c ^ 1][pt][sq][i];
    }
}

void accumulatorAddSub(Colour c, PieceType pt, Square fromSquare, Square toSquare) {
    pt--;
    for (int i = 0; i < LAYER1; i++) {
        accumulator.accumulator[WHITE][i] += network.accumulatorWeights[c][pt][toSquare ^ FLIP_MASK][i] - network.accumulatorWeights[c][pt][fromSquare ^ FLIP_MASK][i];
    }

    for (int i = 0; i < LAYER1; i++) {
        accumulator.accumulator[BLACK][i] += network.accumulatorWeights[c ^ 1][pt][toSquare][i] - network.accumulatorWeights[c ^ 1][pt][fromSquare][i];
    }
}

Score evaluation(Colour stm) {
    Score score = 0;
    for (int i = 0; i < LAYER1; i++) {
        score += SCReLU(accumulator.accumulator[stm][i]) * network.outputWeights[i];
    }

    for (int i = 0; i < LAYER1; i++) {
        score += SCReLU(accumulator.accumulator[stm ^ 1][i]) * network.outputWeights[i + LAYER1];
    }

    score /= QUANTIZATION_A;
    score += network.outputBias;

    return score * SCORE_SCALE / (QUANTIZATION_A * QUANTIZATION_B);
}
