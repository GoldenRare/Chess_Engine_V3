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
    Depth depth;
    uint8_t ageBounds;
} PositionEvaluation;

typedef struct PEBucket {
    PositionEvaluation pe[BUCKET_SIZE]; // TODO: Find optimal size
} PEBucket;

typedef struct TranspositionTable {
    PEBucket *buckets;
    uint64_t mask; // TODO: Could mask be smaller?
    uint8_t age;
} TranspositionTable;
extern TranspositionTable TT;

static inline void startNewSearch() {
    TT.age++;
}

static inline Bound getBound(const PositionEvaluation *pe) {
    return pe->ageBounds & 0x3;
}

static inline Score adjustNodeScoreToTT(Score nodeScore, int ply) {
    return nodeScore >=  GUARANTEE_CHECKMATE ? nodeScore + ply
         : nodeScore <= -GUARANTEE_CHECKMATE ? nodeScore - ply
         : nodeScore;
}

static inline Score adjustNodeScoreFromTT(Score nodeScore, int ply) {
    return nodeScore >=  GUARANTEE_CHECKMATE ? nodeScore - ply
         : nodeScore <= -GUARANTEE_CHECKMATE ? nodeScore + ply
         : nodeScore;
}

// TODO: Need to ensure that function is called correctly due to type conversions
static inline void savePositionEvaluation(PositionEvaluation *pe, Key positionKey, Move bestMove, Depth depth, Bound bound, int16_t nodeScore) {
    uint16_t positionKeyIndex = positionKey >> 48;
    // TODO: How much to value an exact bound? Or even potentially other bounds?
    // Protect more valuable data from being overwritten
    if (positionKeyIndex != pe->key || depth > pe->depth) {
        pe->bestMove = bestMove;
        pe->depth = depth;
        pe->ageBounds = bound;
        pe->nodeScore = nodeScore;
    }
    pe->ageBounds = TT.age << 2 | getBound(pe);
    pe->key = positionKeyIndex;
}

void initializeTranspositionTable();
void setTranspositionTableSize(size_t MB);
PositionEvaluation* probeTranspositionTable(Key positionKey, bool *restrict hasEvaluation);

#endif
