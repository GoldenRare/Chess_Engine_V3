#ifndef UTILITY_H
#define UTILITY_H

#define A8_BB 0x8000000000000000ULL
#define ENTIRE_BOARD 0xFFFFFFFFFFFFFFFFULL

#define RANK_1_BB 0x00000000000000FFULL
#define RANK_2_BB 0x000000000000FF00ULL
#define RANK_3_BB 0x0000000000FF0000ULL
#define RANK_4_BB 0x00000000FF000000ULL
#define RANK_5_BB 0x000000FF00000000ULL
#define RANK_6_BB 0x0000FF0000000000ULL
#define RANK_7_BB 0x00FF000000000000ULL
#define RANK_8_BB 0xFF00000000000000ULL

#define FILE_A_BB 0x8080808080808080ULL
#define FILE_B_BB 0x4040404040404040ULL
#define FILE_C_BB 0x2020202020202020ULL
#define FILE_D_BB 0x1010101010101010ULL
#define FILE_E_BB 0x0808080808080808ULL
#define FILE_F_BB 0x0404040404040404ULL
#define FILE_G_BB 0x0202020202020202ULL
#define FILE_H_BB 0x0101010101010101ULL

#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>

typedef uint64_t Bitboard;
typedef uint64_t Key;
typedef uint16_t Move;

enum PieceType {
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, ALL_PIECES, PIECE_TYPES, NO_PIECE,
    COLOUR_OFFSET = 6
};
typedef enum PieceType PieceType;

enum Colour {
    WHITE, BLACK, COLOURS
};
typedef enum Colour Colour;

enum Direction {
    NORTH = 8, SOUTH = -8, EAST = -1, WEST = 1,
    NORTH_EAST = 7, NORTH_WEST = 9, SOUTH_EAST = -9, SOUTH_WEST = -7
};
typedef enum Direction Direction;

enum Square {
    A8, B8, C8, D8, E8, F8, G8, H8,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A1, B1, C1, D1, E1, F1, G1, H1,
    SQUARES, NO_SQUARE
};
typedef enum Square Square;

enum MoveType {
    QUIET, DOUBLE_PAWN_PUSH, KINGSIDE_CASTLE, QUEENSIDE_CASTLE, CAPTURE, EN_PASSANT_CAPTURE, 
    PROMOTION = 8, KNIGHT_PROMOTION = 8, BISHOP_PROMOTION, ROOK_PROMOTION, QUEEN_PROMOTION, 
    KNIGHT_PROMOTION_CAPTURE, BISHOP_PROMOTION_CAPTURE, ROOK_PROMOTION_CAPTURE, QUEEN_PROMOTION_CAPTURE,
    PROMOTION_PIECE_OFFSET_MASK = 3
};
typedef enum MoveType MoveType;

enum Rank {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANKS
};
typedef enum Rank Rank;

enum File {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILES
};
typedef enum File File;

enum CastlingRights {
    WHITE_KINGSIDE = 1, WHITE_QUEENSIDE = 2, BLACK_KINGSIDE = 4, BLACK_QUEENSIDE = 8,
    WHITE_RIGHTS = WHITE_KINGSIDE | WHITE_QUEENSIDE,
    BLACK_RIGHTS = BLACK_KINGSIDE | BLACK_QUEENSIDE,
    KINGSIDE  = WHITE_KINGSIDE  | BLACK_KINGSIDE,
    QUEENSIDE = WHITE_QUEENSIDE | BLACK_QUEENSIDE,
    ALL_RIGHTS = WHITE_RIGHTS | BLACK_RIGHTS,
    CASTLING_SIDES = 2
};
typedef enum CastlingRights CastlingRights;

inline Rank squareToRank(Square sq) {
    return RANK_8 - (sq >> 3);
}

inline File squareToFile(Square sq) {
    return sq & 7;
}

inline Bitboard rankBitboardOfSquare(Square sq) {
    return RANK_1_BB << squareToRank(sq) * NORTH;
}

inline Bitboard fileBitboardOfSquare(Square sq) {
    return FILE_A_BB >> squareToFile(sq);
}

inline Bitboard squareToBitboard(Square sq) {
    return A8_BB >> sq;
}

inline Square moveSquareInDirection(Square sq, Direction d) {
    return sq - d;
} 

inline Bitboard shiftBitboard(Bitboard b, Direction d) {
    return d > 0 ? b << d : b >> -d;
}

/* If multiple bits are set, returns square of Least Significant Bit. Undefined for b == 0. */
inline Square bitboardToSquare(Bitboard b) {
    return H1 - __builtin_ctzll(b);
}

/* If multiple bits are set, returns square of Least Significant Bit and removes the LSB from pointer. Undefined for b == 0. */
inline Square bitboardToSquareWithReset(Bitboard *b) {
    Square sq = H1 - __builtin_ctzll(*b);
    *b = _blsr_u64(*b);
    return sq;
}

inline void setMove(Move *move, Square fromSquare, Square toSquare, MoveType moveType) {
    *move = moveType << 12 | toSquare << 6 | fromSquare;
}

inline Square getFromSquare(const Move *move) {
    return *move & 0x3F;
}

inline Square getToSquare(const Move *move) {
    return *move >> 6 & 0x3F;
}

inline MoveType getMoveType(const Move *move) {
    return *move >> 12;
}

inline int populationCount(Bitboard b) {
    return __builtin_popcountll(b);
}

/* Parallel Bits Extract */
inline uint64_t pext(uint64_t src, uint64_t mask) {
    return _pext_u64(src, mask);
}

inline int max(int a, int b) {
    return a >= b ? a : b;
}

#endif