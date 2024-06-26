#ifndef MOVE_GENERATOR_H
#define MOVE_GENERATOR_H

#define NUMBER_OF_SLIDING_DIRECTIONS 4
#define MAX_SLIDING_ATTACKS 107648
#define MAX_MOVES 256

#include <stdbool.h>
#include <stdint.h>
#include "chess_board.h"
#include "utility.h"

typedef struct Magic {
    Bitboard mask;
    uint32_t offset;
} Magic;

enum MagicIndex {
    BISHOP_INDEX, ROOK_INDEX, MAGIC_INDICES
};
typedef enum MagicIndex MagicIndex;

/* Does not include pawns */
enum NonSliderAttacker {
    KNIGHT_ATTACKER, KING_ATTACKER, NON_SLIDER_ATTACKERS
};
typedef enum NonSliderAttacker NonSliderAttacker;

enum MoveGenerationStage {
    CAPTURES, NON_CAPTURES
};
typedef enum MoveGenerationStage MoveGenerationStage;

extern Magic magicTable[MAGIC_INDICES][SQUARES];
extern Bitboard pawnAttacks[COLOURS][SQUARES];
extern Bitboard nonSlidingAttacks[NON_SLIDER_ATTACKERS][SQUARES]; // Does not include pawns
extern Bitboard slidingAttacks[MAX_SLIDING_ATTACKS];

inline Bitboard getPawnAttacks(Colour c, Square sq) {
    return pawnAttacks[c][sq];
}

inline Bitboard getNonSlidingAttacks(NonSliderAttacker nonSliderAttacker, Square sq) {
    return nonSlidingAttacks[nonSliderAttacker][sq];
}

inline Bitboard getSlidingAttacks(Bitboard occupied, Square sq, MagicIndex index) {
    const Magic *magic = &magicTable[index][sq];
    return slidingAttacks[magic->offset + pext(occupied, magic->mask)];
}

inline Bitboard getAttacks(PieceType pt, Bitboard occupied, Square sq) {
    return pt == KNIGHT ? getNonSlidingAttacks(KNIGHT_ATTACKER, sq) 
         : pt == BISHOP ? getSlidingAttacks(occupied, sq, BISHOP_INDEX)
         : pt == ROOK   ? getSlidingAttacks(occupied, sq, ROOK_INDEX)  
         : pt == QUEEN  ? getSlidingAttacks(occupied, sq, BISHOP_INDEX) | getSlidingAttacks(occupied, sq, ROOK_INDEX)
         :                getNonSlidingAttacks(KING_ATTACKER, sq);
}

inline MoveObject* generatePromotions(MoveObject *moveList, Square fromSq, Square toSq) { 
    setMove(moveList++, fromSq, toSq, QUEEN_PROMOTION );
    setMove(moveList++, fromSq, toSq, ROOK_PROMOTION  );
    setMove(moveList++, fromSq, toSq, BISHOP_PROMOTION);
    setMove(moveList++, fromSq, toSq, KNIGHT_PROMOTION);
    return moveList;
}

void initializeMoveGenerator();
void initializeNonSlidingAttacks();
void initializeSlidingAttacks();

Bitboard generateKnightAttacks(Bitboard knightSq);
Bitboard generateSlidingAttacks(const Direction directions[], size_t numDirections, Square sq, Bitboard occupied);
Bitboard generateKingAttacks(Bitboard kingSq);

MoveObject* createMoveList(const ChessBoard *board, MoveObject *moveList, MoveGenerationStage stage);
MoveObject* generateNonKingMoves(const ChessBoard *board, MoveObject *moveList, Bitboard validSquares, MoveGenerationStage stage);
MoveObject* generatePawnMoves(const ChessBoard *board, MoveObject *moveList, Bitboard validSquares, MoveGenerationStage stage);
MoveObject* generatePieceMoves(const ChessBoard *board, MoveObject *moveList, Bitboard validSquares, PieceType pt);
MoveObject* generateCastleMoves(const ChessBoard *board, MoveObject *moveList);

#endif