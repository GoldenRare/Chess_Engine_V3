#ifndef SEARCH_H
#define SEARCH_H

#include "chess_board.h"
#include "utility.h"

constexpr Depth MAX_DEPTH = 8;

MoveObject startSearch(ChessBoard *restrict board, Depth depth);

#endif
