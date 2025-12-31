#include <stdint.h>
#include "nnue.h"
#include "utility.h"

constexpr int FLIP_MASK       = 0b111000;
constexpr int SCORE_SCALE     =      400;
constexpr int QUANTIZATION_A  =      255;
constexpr int QUANTIZATION_B  =       64;
constexpr int PERSPECTIVE     =        2;

typedef struct Network {
    int16_t accumulatorWeights[COLOURS][PIECE_TYPES - 1][SQUARES][LAYER1];
    int16_t accumulatorBiases[LAYER1];

    int16_t outputWeights[LAYER1 * PERSPECTIVE];
    int16_t outputBias;
} Network;

static const uint8_t networkData[] = {
    #embed "nnue.bin"
};

static const Network *network = (const Network *) networkData;

static inline int32_t SCReLU(int16_t val) {
    int16_t clamped = val >= QUANTIZATION_A ? QUANTIZATION_A
                    : val >  0              ? val
                    : 0;
    return (int32_t) clamped * clamped;
}

void accumulatorReset(Accumulator *restrict accumulator) {
    for (int i = 0; i < LAYER1; i++) {
        accumulator->accumulator[WHITE][i] = network->accumulatorBiases[i];
        accumulator->accumulator[BLACK][i] = network->accumulatorBiases[i];
    }
}

void accumulatorAdd(Accumulator *restrict accumulator, Colour c, PieceType pt, Square sq) {
    pt--;
    for (int i = 0; i < LAYER1; i++) accumulator->accumulator[WHITE][i] += network->accumulatorWeights[c    ][pt][sq ^ FLIP_MASK][i];
    for (int i = 0; i < LAYER1; i++) accumulator->accumulator[BLACK][i] += network->accumulatorWeights[c ^ 1][pt][sq            ][i];
}

void accumulatorSub(Accumulator *restrict accumulator, Colour c, PieceType pt, Square sq) {
    pt--;
    for (int i = 0; i < LAYER1; i++) accumulator->accumulator[WHITE][i] -= network->accumulatorWeights[c    ][pt][sq ^ FLIP_MASK][i];
    for (int i = 0; i < LAYER1; i++) accumulator->accumulator[BLACK][i] -= network->accumulatorWeights[c ^ 1][pt][sq            ][i];
}

void accumulatorAddSub(Accumulator *restrict accumulator, Colour c, PieceType pt, Square fromSquare, Square toSquare) {
    pt--;
    for (int i = 0; i < LAYER1; i++)
        accumulator->accumulator[WHITE][i] += network->accumulatorWeights[c][pt][toSquare ^ FLIP_MASK][i] - network->accumulatorWeights[c][pt][fromSquare ^ FLIP_MASK][i];

    for (int i = 0; i < LAYER1; i++)
        accumulator->accumulator[BLACK][i] += network->accumulatorWeights[c ^ 1][pt][toSquare][i] - network->accumulatorWeights[c ^ 1][pt][fromSquare][i];
}

Score evaluation(const Accumulator *restrict accumulator, Colour stm) {
    Score score = 0;
    for (int i = 0; i < LAYER1; i++) score += SCReLU(accumulator->accumulator[stm    ][i]) * network->outputWeights[i         ];
    for (int i = 0; i < LAYER1; i++) score += SCReLU(accumulator->accumulator[stm ^ 1][i]) * network->outputWeights[i + LAYER1];

    score /= QUANTIZATION_A;
    score += network->outputBias;

    return score * SCORE_SCALE / (QUANTIZATION_A * QUANTIZATION_B);
}
