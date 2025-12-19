#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "utility.h"

constexpr int BUCKET_SIZE = 3; // TODO: Find optimal size

typedef struct PositionEvaluation {
    uint16_t key;
    int16_t nodeScore;
    int16_t staticEvaluation;
    Move bestMove;
    Depth depth;
    uint8_t ageBounds;
} PositionEvaluation;

typedef struct PositionEvaluationBucket {
    PositionEvaluation pe[BUCKET_SIZE];
} PEBucket;

typedef struct TranspositionTable {
    PEBucket *buckets;
    uint64_t mask; // TODO: Could mask be smaller?
    uint8_t age;
} TT;

// Can be called multiple times but the first call must have tt->buckets == nullptr for the forced call to free()
static inline void createTranspositionTable(TT *restrict tt, size_t MB) {
    size_t numberOfBuckets = MB * 1024 * 1024 / sizeof(PEBucket); // Convert MB to B
    
    // Rounds down to the nearest largest power of 2, this may cause substantially less space allocation than what was requested
    numberOfBuckets = squareToBitboard(bitboardToSquareMSB(numberOfBuckets));
    free(tt->buckets);
    tt->buckets = calloc(numberOfBuckets, sizeof(PEBucket));
    tt->mask = numberOfBuckets - 1;
    tt->age = -1;
}

static inline void clearTranspositionTable(TT *restrict tt) {
    memset(tt->buckets, 0, sizeof(PEBucket) * (tt->mask + 1));
    tt->age = -1;
}

static inline void destroyTranspositionTable(TT *restrict tt) {
    free(tt->buckets);
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
static inline void savePositionEvaluation(TT *tt, PositionEvaluation *pe, Key positionKey, Move bestMove, Depth depth, Bound bound, int16_t nodeScore, int16_t staticEvaluation) {
    uint16_t positionKeyIndex = positionKey >> 48;
    // TODO: How much to value an exact bound? Or even potentially other bounds?
    // Protect more valuable data from being overwritten
    if (positionKeyIndex != pe->key || depth > pe->depth) {
        pe->bestMove = bestMove;
        pe->depth = depth;
        pe->ageBounds = bound;
        pe->nodeScore = nodeScore;
    }
    pe->staticEvaluation = staticEvaluation;
    pe->ageBounds = tt->age << 2 | getBound(pe);
    pe->key = positionKeyIndex;
}

// TODO: Consider thread safety
static inline PositionEvaluation* probeTranspositionTable(const TT *tt, Key positionKey, bool *restrict hasEvaluation) {
    PositionEvaluation* pe = &tt->buckets[positionKey & tt->mask].pe[0];
    uint16_t keyIndex = positionKey >> 48;

    for (int i = 0; i < BUCKET_SIZE; i++) {
        if (pe[i].key == keyIndex || !pe[i].key) {
            *hasEvaluation = pe[i].key;
            return &pe[i];
        }
    }

    // Depth preferred replacement
    PositionEvaluation *replace = pe;
    for (int i = 1; i < BUCKET_SIZE; i++)
        if (replace->depth > pe[i].depth) replace = &pe[i];
    *hasEvaluation = false;
    return replace;
}

#endif
