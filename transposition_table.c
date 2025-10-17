#include <stdint.h>
#include "transposition_table.h"
#include "utility.h"

PositionEvaluation* probeTranspositionTable(const TT *tt, Key positionKey, bool *restrict hasEvaluation) {
    PositionEvaluation* pe = &tt->buckets[positionKey & tt->mask].pe[0];
    uint16_t keyIndex = positionKey >> 48;

    for (int i = 0; i < BUCKET_SIZE; i++) {
        if (pe[i].key == keyIndex || !pe[i].key) {
            *hasEvaluation = pe[i].key;
            return &pe[i];
        }
    }

    // TODO: Implement replacement scheme
    *hasEvaluation = false;
    return &pe[0];
}
