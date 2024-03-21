#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include <stdbool.h>
#include "utility.h"

typedef struct ChessBoard {
    Key positionKey;
    Bitboard checkers;
    Bitboard pieces[COLOURS][PIECE_TYPES];
    PieceType pieceTypes[SQUARES];
    Colour sideToMove;
    Square enPassant;
    CastlingRights castlingRights;
} ChessBoard;

// Keeps track of the information that is lost when a move is made
typedef struct IrreversibleBoardState {
    Key positionKey;
    Bitboard checkers;
    PieceType capturedPiece;
    Square enPassant;
    CastlingRights castlingRights;
} IrreversibleBoardState;

// Indexing the same square will return 0. Example: fullLine[e4][e4] == 0
extern Bitboard fullLine[SQUARES][SQUARES];

// Includes the endpoints as well
extern Bitboard inBetweenLine[SQUARES][SQUARES];

extern const char *SQUARE_NAME[];
extern const char PROMOTION_NAME[];

inline Bitboard getPieces(const ChessBoard *board, Colour c, PieceType pt) {
    return board->pieces[c][pt];
}

inline Bitboard getOccupiedSquares(const ChessBoard *board) {
    return board->pieces[WHITE][ALL_PIECES] | board->pieces[BLACK][ALL_PIECES];
}

inline Square getKingSquare(const ChessBoard *board, Colour c) {
    return bitboardToSquare(getPieces(board, c, KING));
}

inline bool isPathClear(Square from, Square to, Bitboard occupied) {
    return !(inBetweenLine[from][to] & occupied);
}

void initializeChessBoard();

void parseFEN(ChessBoard *board, const char *fenString);
void addPiece(ChessBoard *board, Colour c, PieceType pt, Square sq);

// Assumes that the piece is moving to an EMPTY square
void movePiece(ChessBoard *board, Colour c, PieceType pt, Square fromSquare, Square toSquare); 

void removePiece(ChessBoard *board, Colour c, PieceType pt, Square sq);
void makeMove(ChessBoard *board, const Move *move, IrreversibleBoardState *ibs);
void undoMove(ChessBoard *board, const Move *move, const IrreversibleBoardState *ibs);
bool isLegalMove(const ChessBoard *board, const Move *move, Bitboard pinned);
bool isPseudoMove(const ChessBoard *board, Move move);

Bitboard getPinnedPieces(const ChessBoard *board);
Bitboard attackersTo(const ChessBoard *board, Square sq, Colour attackedSide, Bitboard occupied);

void printBitboard(Bitboard b);

#endif