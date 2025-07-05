#include <stdint.h>
#include <stdio.h>
#include "nnue.h"
#include "utility.h"

static Network network; // TODO: Can we make it const?

static inline int32_t SCReLU(int16_t val) {
    return val >= QUANTIZATION_A ? QUANTIZATION_A * QUANTIZATION_A 
         : val >  0              ? val * val
         : 0; 
}

void initializeNNUE() {
    FILE *file = fopen("nnue.bin", "rb");
    fread(&network, sizeof(Network), 1 , file);
    fclose(file);
}

void accumulatorReset(Accumulator *restrict accumulator) {
    for (int i = 0; i < LAYER1; i++) {
        accumulator->accumulator[WHITE][i] = network.accumulatorBiases[i];
        accumulator->accumulator[BLACK][i] = network.accumulatorBiases[i];
    }
}

void accumulatorAdd(Accumulator *restrict accumulator, Colour c, PieceType pt, Square sq) {
    pt--;
    for (int i = 0; i < LAYER1; i++) {
        accumulator->accumulator[WHITE][i] += network.accumulatorWeights[c][pt][sq ^ FLIP_MASK][i];
    }

    for (int i = 0; i < LAYER1; i++) {
        accumulator->accumulator[BLACK][i] += network.accumulatorWeights[c ^ 1][pt][sq][i];
    }
}

void accumulatorSub(Accumulator *restrict accumulator, Colour c, PieceType pt, Square sq) {
    pt--;
    for (int i = 0; i < LAYER1; i++) {
        accumulator->accumulator[WHITE][i] -= network.accumulatorWeights[c][pt][sq ^ FLIP_MASK][i];
    }

    for (int i = 0; i < LAYER1; i++) {
        accumulator->accumulator[BLACK][i] -= network.accumulatorWeights[c ^ 1][pt][sq][i];
    }
}

void accumulatorAddSub(Accumulator *restrict accumulator, Colour c, PieceType pt, Square fromSquare, Square toSquare) {
    pt--;
    for (int i = 0; i < LAYER1; i++) {
        accumulator->accumulator[WHITE][i] += network.accumulatorWeights[c][pt][toSquare ^ FLIP_MASK][i] - network.accumulatorWeights[c][pt][fromSquare ^ FLIP_MASK][i];
    }

    for (int i = 0; i < LAYER1; i++) {
        accumulator->accumulator[BLACK][i] += network.accumulatorWeights[c ^ 1][pt][toSquare][i] - network.accumulatorWeights[c ^ 1][pt][fromSquare][i];
    }
}

Score evaluation(const Accumulator *restrict accumulator, Colour stm) {
    Score score = 0;
    for (int i = 0; i < LAYER1; i++) {
        score += SCReLU(accumulator->accumulator[stm][i]) * network.outputWeights[i];
    }

    for (int i = 0; i < LAYER1; i++) {
        score += SCReLU(accumulator->accumulator[stm ^ 1][i]) * network.outputWeights[i + LAYER1];
    }

    score /= QUANTIZATION_A;
    score += network.outputBias;

    return score * SCORE_SCALE / (QUANTIZATION_A * QUANTIZATION_B);
}
