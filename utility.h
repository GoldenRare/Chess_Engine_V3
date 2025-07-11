#ifndef UTILITY_H
#define UTILITY_H

#include <stddef.h>
#include <stdint.h>
#include <immintrin.h>

typedef uint64_t Bitboard;
typedef uint64_t Key;
typedef uint16_t Move;
typedef uint8_t Depth;

typedef int32_t Score; // TODO: Could be the Value enum

typedef struct MoveObject {
    Move move;
    int16_t score;
} MoveObject;

typedef enum PieceType {
    NO_PIECE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, PIECE_TYPES,
    ALL_PIECES = 0, COLOUR_OFFSET = 6
} PieceType;

typedef enum Colour {
    WHITE, BLACK, COLOURS
} Colour;

typedef enum Direction {
    NORTH = 8, SOUTH = -8, EAST = -1, WEST = 1,
    NORTH_EAST = 7, NORTH_WEST = 9, SOUTH_EAST = -9, SOUTH_WEST = -7
} Direction;

typedef enum Square {
    A8, B8, C8, D8, E8, F8, G8, H8,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A1, B1, C1, D1, E1, F1, G1, H1,
    SQUARES, NO_SQUARE
} Square;

typedef enum MoveType { // TODO
    QUIET, DOUBLE_PAWN_PUSH, CASTLE, EN_PASSANT = 4, PROMOTION = 8,
    KNIGHT_PROMOTION = 8, BISHOP_PROMOTION, ROOK_PROMOTION, QUEEN_PROMOTION,
    PROMOTION_PIECE_MASK = 3
} MoveType;

typedef enum Rank {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANKS
} Rank;

typedef enum File {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILES
} File;

typedef enum CastlingRights {
    WHITE_KINGSIDE = 1, WHITE_QUEENSIDE = 2, BLACK_KINGSIDE = 4, BLACK_QUEENSIDE = 8,
    WHITE_RIGHTS = WHITE_KINGSIDE | WHITE_QUEENSIDE,
    BLACK_RIGHTS = BLACK_KINGSIDE | BLACK_QUEENSIDE,
    KINGSIDE  = WHITE_KINGSIDE  | BLACK_KINGSIDE,
    QUEENSIDE = WHITE_QUEENSIDE | BLACK_QUEENSIDE,
    ALL_RIGHTS = WHITE_RIGHTS | BLACK_RIGHTS,
    CASTLING_SIDES = 2
} CastlingRights;

typedef enum Value {
    DRAW = 0, GUARANTEE_CHECKMATE = 31500, CHECKMATE = 32000, INFINITE = 32767 // TODO: Range of guarantee checkmate
} Value;

typedef enum Bound {
    LOWER, EXACT, UPPER
} Bound;

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

constexpr Move NO_MOVE = 0;

constexpr char START_POS[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr char SQUARE_NAME[][3] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8", 
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", 
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", 
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", 
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", 
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", 
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", 
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"
};
constexpr int MAX_MOVES = 256;

static inline Rank squareToRank(Square sq) {
    return RANK_8 - (sq >> 3);
}

static inline File squareToFile(Square sq) {
    return sq & 7;
}

static inline Bitboard rankBitboardOfSquare(Square sq) {
    return RANK_1_BB << squareToRank(sq) * NORTH;
}

static inline Bitboard fileBitboardOfSquare(Square sq) {
    return FILE_A_BB >> squareToFile(sq);
}

static inline Bitboard squareToBitboard(Square sq) {
    return A8_BB >> sq;
}

static inline Square moveSquareInDirection(Square sq, Direction d) {
    return sq - d;
} 

static inline Bitboard shiftBitboard(Bitboard b, Direction d) {
    return d > 0 ? b << d : b >> -d;
}

/* If multiple bits are set, returns square of Most Significant Bit. Undefined for b == 0. */
static inline Square bitboardToSquareMSB(Bitboard b) {
    return __builtin_clzll(b);
}

/* If multiple bits are set, returns square of Least Significant Bit. Undefined for b == 0. */
static inline Square bitboardToSquare(Bitboard b) {
    return H1 - __builtin_ctzll(b);
}

/* If multiple bits are set, returns square of Least Significant Bit and removes the LSB from pointer. Undefined for b == 0. */
static inline Square bitboardToSquareWithReset(Bitboard *b) {
    Square sq = H1 - __builtin_ctzll(*b);
    *b = _blsr_u64(*b);
    return sq;
}

static inline void setMove(MoveObject *move, Square fromSquare, Square toSquare, MoveType moveType) {
    move->move = moveType << 12 | toSquare << 6 | fromSquare;
}

static inline Square getFromSquare(Move move) {
    return move & 0x3F;
}

static inline Square getToSquare(Move move) {
    return move >> 6 & 0x3F;
}

static inline MoveType getMoveType(Move move) {
    return move >> 12;
}

static inline int populationCount(Bitboard b) {
    return __builtin_popcountll(b);
}

/* Parallel Bits Extract */
static inline uint64_t pext(uint64_t src, uint64_t mask) {
    return _pext_u64(src, mask);
}

static inline int max(int a, int b) {
    return a >= b ? a : b;
}

static inline bool isAdjacentSquare(Square fromSq, Square toSq) {
    int rankDistance = abs((int) squareToRank(toSq) - (int) squareToRank(fromSq));
    int fileDistance = abs((int) squareToFile(toSq) - (int) squareToFile(fromSq));
    return max(rankDistance, fileDistance) == 1;
}

// A PRNG that can be found here: https://arxiv.org/pdf/1402.6246
static inline uint64_t random64BitNumber(uint64_t *restrict seed) {
    *seed ^= *seed >> 12, *seed ^= *seed << 25, *seed ^= *seed >> 27;
    return *seed * 2685821657736338717ULL;
}

// A PRNG that can be found here: https://prng.di.unimi.it/splitmix64.c
static inline uint64_t splitMix64(uint64_t *restrict seed) {
	uint64_t z = *seed += 0x9E3779B97F4A7C15ULL;
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	return z ^ (z >> 31);
}

// Returns the length of the string
static inline size_t moveToString(char *restrict destination, Move move) {
    constexpr char PROMOTION_PIECE[] = {'n', 'b', 'r', 'q'};
    Square fromSquare = getFromSquare(move);
    Square toSquare = getToSquare(move);
    MoveType moveType = getMoveType(move);
    bool isPromotion = moveType & PROMOTION;

    destination[0] = SQUARE_NAME[fromSquare][0];
    destination[1] = SQUARE_NAME[fromSquare][1];
    destination[2] = SQUARE_NAME[toSquare  ][0];
    destination[3] = SQUARE_NAME[toSquare  ][1];
    destination[4] = isPromotion ? PROMOTION_PIECE[moveType & PROMOTION_PIECE_MASK] : '\0';
    destination[5] = '\0';
    return isPromotion ? 5 : 4;
}

#endif
