#ifndef MOVE_GENERATOR_H
#define MOVE_GENERATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "chess_board.h"
#include "utility.h"

extern Bitboard knightAttacks[SQUARES];

void initializeMoveGenerator();
void initializeKnightAttacks();
Bitboard generateSlidingAttacks(const Direction directions[], size_t numDirections, Square sq, Bitboard occupied);
/* Essentially checks if fromSq and toSq are within a king ring distance from each other.*/
bool isDirectionMaintained(Square fromSq, Square toSq); 

#endif