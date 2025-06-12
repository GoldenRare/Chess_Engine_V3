#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include <stddef.h>
#include <stdint.h>
#include "utility.h"

constexpr int BUCKET_SIZE = 3; // TODO: Find optimal size

typedef struct PositionEvaluation {
    uint16_t key;
    int16_t nodeScore;
    Move bestMove;
    uint8_t depth; // TODO: Consider QSearch depth, it is not unsigned since QSearch depth is negative
    uint8_t ageBounds; // TODO: Does not consider aging
} PositionEvaluation;

typedef struct PEBucket {
    PositionEvaluation pe[BUCKET_SIZE]; // TODO: Find optimal size
} PEBucket;

typedef struct TranspositionTable {
    PEBucket *buckets;
    uint64_t mask; // TODO: Could mask be smaller?
} TranspositionTable;
extern TranspositionTable TT;

static inline Bound getBound(const PositionEvaluation *pe) {
    return pe->ageBounds & 0x3;
}

// TODO: Need to ensure that function is called correctly due to type conversions
static inline void savePositionEvaluation(PositionEvaluation *pe, Key positionKey, Move bestMove, uint8_t depth, Bound bound, int16_t nodeScore) {
    pe->key = positionKey >> 48;
    pe->bestMove = bestMove;
    pe->depth = depth; // TODO: Consider QSearch depth, it is not unsigned since QSearch depth is negative
    pe->ageBounds = bound;
    pe->nodeScore = nodeScore;
}

void initializeTranspositionTable();
void setTranspositionTableSize(size_t MB);
PositionEvaluation* probeTranspositionTable(Key positionKey, bool *restrict hasEvaluation);

#endif
