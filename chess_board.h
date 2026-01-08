#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include <stdint.h>
#include "utility.h"
#include "nnue.h"

// Uses a linked list to keep track of the history of the game. 
// Maintains information that is lost when a move is made but also
// information that is expensive to compute so instead of recomputing it is saved. 
typedef struct ChessBoardHistory {
    struct ChessBoardHistory *previous; // TODO: Consider making it an array in the struct, or pointer that is sequential
    Key positionKey;
    Bitboard checkers;
    Bitboard pinnedPieces;
    PieceType capturedPiece;
    Square enPassant;
    CastlingRights castlingRights;
    uint8_t halfmoveClock; // TODO: Maybe the type
} ChessBoardHistory;

typedef struct ChessBoard {
    Accumulator accumulator;
    ChessBoardHistory *history;
    Bitboard pieces[COLOURS][PIECE_TYPES];
    PieceType pieceTypes[SQUARES];
    Colour sideToMove;
    uint16_t ply; // TODO: Maybe the type
} ChessBoard;

// Indexing the same square will return 0. Example: fullLine[e4][e4] == 0
extern Bitboard fullLine[SQUARES][SQUARES];

// Includes the endpoints as well
extern Bitboard inBetweenLine[SQUARES][SQUARES];

static inline Key getPositionKey(const ChessBoard *restrict board) {
    return board->history->positionKey;
}

static inline Square getEnPassant(const ChessBoard *restrict board) {
    return board->history->enPassant;
}

static inline Bitboard getCheckers(const ChessBoard *restrict board) {
    return board->history->checkers;
}

static inline Bitboard getPieces(const ChessBoard *restrict board, Colour c, PieceType pt) {
    return board->pieces[c][pt];
}

static inline Bitboard getBothPieces(const ChessBoard *restrict board, PieceType pt) {
    return board->pieces[WHITE][pt] | board->pieces[BLACK][pt];
}

// TODO: Is it worth having one lookup rather than ORing?
static inline Bitboard getOccupiedSquares(const ChessBoard *restrict board) {
    return board->pieces[WHITE][ALL_PIECES] | board->pieces[BLACK][ALL_PIECES];
}

// TODO: Is it worth maintaining the king square in struct rather than compute?
static inline Square getKingSquare(const ChessBoard *restrict board, Colour c) {
    return bitboardToSquare(getPieces(board, c, KING));
}

static inline bool hasNonPawnMaterial(const ChessBoard *restrict board, Colour c) {
    return board->pieces[c][ALL_PIECES] ^ board->pieces[c][PAWN] ^ board->pieces[c][KING];
}

// TODO: Include more scenarios if necessary
static inline bool insufficientMaterial(const ChessBoard *restrict board) {
    return !(getBothPieces(board, PAWN) | getBothPieces(board, ROOK) | getBothPieces(board, QUEEN)) && populationCount(getOccupiedSquares(board)) < 4;
}

// TODO: For search, but repetition by history vs repetition by search tree transpose
// Checks for twofold repetition
static inline bool isRepetition(const ChessBoard *restrict board) {
    if (board->history->halfmoveClock >= 4) {
        Key current = getPositionKey(board);
        ChessBoardHistory *previous = board->history->previous->previous->previous->previous;
        if (current == previous->positionKey) return true;
        for (int count = previous->halfmoveClock; count >= 2; count -= 2) {
            previous = previous->previous->previous;
            if (current == previous->positionKey) return true;
        }
    }
    return false;
}

static inline void undoNullMove(ChessBoard *restrict board) {
    board->sideToMove ^= 1;
    board->history = board->history->previous;
}

void initializeChessBoard();

void parseFEN(ChessBoard *restrict board, ChessBoardHistory *restrict history, const char *restrict fen);
void getFEN(const ChessBoard *restrict board, char *restrict destination);

void makeNullMove(ChessBoard *board, ChessBoardHistory *newState);
void makeMove(ChessBoard *board, ChessBoardHistory *newState, Move move);
void undoMove(ChessBoard *restrict board, Move move);
bool isDraw(const ChessBoard *restrict board);
bool isLegalMove(const ChessBoard *restrict board, Move move);
bool isPseudoMove(const ChessBoard *restrict board, Move move);

#endif
