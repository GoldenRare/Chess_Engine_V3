#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#define BUCKET_SIZE 3

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "utility.h"

typedef struct PositionEvaluation {
    uint16_t key;
    uint16_t bestMove;
    uint8_t depth;
    uint8_t ageBounds;
    int16_t nodeScore;
} PositionEvaluation;

typedef struct Bucket {
    PositionEvaluation pe[BUCKET_SIZE];
} Bucket;

typedef struct TranspositionTable {
    uint64_t mask; // TODO: Could mask be smaller?
    Bucket *table;
} TranspositionTable;

extern TranspositionTable TT;

inline Bound getBound(const PositionEvaluation *pe) {
    return pe->ageBounds & 0x3;
}

// TODO: Need to ensure that function is called correctly due to type conversions
inline void savePositionEvaluation(PositionEvaluation *pe, Key positionKey, Move bestMove, uint8_t depth, Bound bound, int16_t nodeScore) {
    pe->key = positionKey >> 48;
    pe->bestMove = bestMove;
    pe->depth = depth; // TODO: Consider QSearch depth, it is not unsigned since QSearch depth is negative
    pe->ageBounds = bound; // TODO: Does not consider aging
    pe->nodeScore = nodeScore;
}

void setTranspositionTableSize(size_t MB);
PositionEvaluation* probeTranspositionTable(Key positionKey, bool *hasEvaluation);

#endif