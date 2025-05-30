#ifndef UTILITY_H
#define UTILITY_H

#include <stdint.h>
#include <immintrin.h>

typedef uint64_t Bitboard;
typedef uint64_t Key;
typedef uint16_t Move;
typedef int Depth;

enum PieceType {
    NO_PIECE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, ALL_PIECES, PIECE_TYPES,
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
    QUIET, DOUBLE_PAWN_PUSH, CASTLE, EN_PASSANT_CAPTURE = 4,
    PROMOTION = 8, KNIGHT_PROMOTION = 8, BISHOP_PROMOTION, ROOK_PROMOTION, QUEEN_PROMOTION,
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

enum Value {
    CHECKMATED = -32000, DRAW = 0, INFINITE = 32700 // Temporary
};
typedef enum Value Value;

enum Bound {
    UPPER, LOWER, EXACT
};
typedef enum Bound Bound;

constexpr Bitboard RANK_1_BB = 0x00000000000000FFULL;
constexpr Bitboard RANK_2_BB = RANK_1_BB << NORTH;
constexpr Bitboard RANK_4_BB = RANK_2_BB << NORTH * 2;
constexpr Bitboard RANK_5_BB = RANK_4_BB << NORTH;
constexpr Bitboard RANK_7_BB = RANK_5_BB << NORTH * 2;
constexpr Bitboard RANK_8_BB = RANK_7_BB << NORTH;

constexpr Bitboard FILE_A_BB = 0x8080808080808080ULL;
constexpr Bitboard FILE_B_BB = FILE_A_BB >> -EAST;
constexpr Bitboard FILE_G_BB = FILE_B_BB >> -EAST * 5;
constexpr Bitboard FILE_H_BB = FILE_G_BB >> -EAST;

constexpr Bitboard A8_BB = FILE_A_BB & RANK_8_BB;

typedef struct MoveObject {
    Move move;
    uint16_t score;
} MoveObject;

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

/* If multiple bits are set, returns square of Most Significant Bit. Undefined for b == 0. */
inline Square bitboardToSquareMSB(Bitboard b) {
    return __builtin_clzll(b);
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

inline void setMove(MoveObject *move, Square fromSquare, Square toSquare, MoveType moveType) {
    move->move = moveType << 12 | toSquare << 6 | fromSquare;
}

inline Square getFromSquare(const Move move) {
    return move & 0x3F;
}

inline Square getToSquare(const Move move) {
    return move >> 6 & 0x3F;
}

inline MoveType getMoveType(const Move move) {
    return move >> 12;
}

inline int populationCount(Bitboard b) {
    return __builtin_popcountll(b);
}

/* Parallel Bits Extract */
inline uint64_t pext(uint64_t src, uint64_t mask) {
    return _pext_u64(src, mask);
}

// Random number generator derived from Stockfish, more precisely from Sebastiano Vigna (2014).
inline uint64_t random64BitNumber(uint64_t *seed) {
    *seed ^= *seed >> 12, *seed ^= *seed << 25, *seed ^= *seed >> 27;
    return *seed * 2685821657736338717LL;
}

inline int max(int a, int b) {
    return a >= b ? a : b;
}

inline void encodeChessMove(char *destination, const char *fromSquare, const char *toSquare, const char promotionPiece) {
    destination[0] = fromSquare[0];
    destination[1] = fromSquare[1];
    destination[2] = toSquare[0];
    destination[3] = toSquare[1];
    destination[4] = promotionPiece; // This will most often be the null character if the move is no promotion
    destination[5] = '\0';
}

inline bool isAdjacentSquare(Square fromSq, Square toSq) {
    int rankDistance = abs((int) squareToRank(toSq) - (int) squareToRank(fromSq));
    int fileDistance = abs((int) squareToFile(toSq) - (int) squareToFile(fromSq));
    return max(rankDistance, fileDistance) == 1;
}

// TODO
#define NO_MOVE 0

#endif