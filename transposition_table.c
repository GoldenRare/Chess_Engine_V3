#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "transposition_table.h"
#include "utility.h"

TranspositionTable TT;

void setTranspositionTableSize(size_t MB) {
    free(TT.table);
    size_t numberOfBuckets = MB * 1024 * 1024 / sizeof(Bucket); // Convert MB to B then, divide by the size of 1 bucket
    
    // Rounds down to the nearest largest power of 2, this may cause substantially less space allocation than what was requested
    numberOfBuckets = squareToBitboard(bitboardToSquareMSB(numberOfBuckets)); 

    TT.table = (Bucket*) calloc(numberOfBuckets, sizeof(Bucket));
    if (TT.table == NULL) {
        printf("info string Memory Allocation Failed! Exiting ...\n"); 
        exit(1);
    }
    
    printf("info string Allocating %llu MB of memory.\n", numberOfBuckets * sizeof(Bucket) / 1024 / 1024);
    TT.mask = numberOfBuckets - 1;
}

PositionEvaluation* probeTranspositionTable(Key positionKey, bool *hasEvaluation) {
    PositionEvaluation* pe = &TT.table[positionKey & TT.mask].pe[0];
    uint16_t keyIndex = positionKey >> 48;

    for (size_t i = 0; i < BUCKET_SIZE; i++) {
        if (pe[i].key == keyIndex || !pe[i].key) {
            *hasEvaluation = pe[i].key;
            return &pe[i];
        }
    }

    // TODO: Implement replacement scheme
    *hasEvaluation = false;
    return &pe[0];
}