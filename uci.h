#ifndef UCI_H
#define UCI_H

#include <stddef.h>
#include <stdint.h>
#include "chess_board.h"
#include "nnue.h"
#include "transposition_table.h"

typedef struct UCI_Configuration {
    Accumulator accumulator; // TODO: May not be needed
    ChessBoard board; // TODO: May not be needed
    TT tt; // TODO: May not be needed
    size_t hashSize;
    uint8_t threads;
} UCI_Configuration;

void uciLoop();

#endif
