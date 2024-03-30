#ifndef EVALUATION_H
#define EVALUATION_H

#include <stdint.h>
#include "chess_board.h"
#include "utility.h"

typedef int16_t Score;

extern const Score PIECE_VALUE[ALL_PIECES];

int evaluation(const ChessBoard *board);

#endif