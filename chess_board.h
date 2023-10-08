#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include <stdbool.h>
#include "utility.h"

typedef struct ChessBoard {
    Colour sideToMove;
    PieceType pieceTypes[SQUARES];
    Bitboard pieces[COLOURS][PIECE_TYPES];
    Square enPassant;
    CastlingRights castlingRights;
} ChessBoard;

// Keeps track of the information that is lost when a move is made
typedef struct IrreversibleBoardState {
    PieceType capturedPiece;
    Square enPassant;
    CastlingRights castlingRights;
} IrreversibleBoardState;

inline Colour getSideToMove(const ChessBoard *board) {
    return board->sideToMove;
}

inline Bitboard getPieces(const ChessBoard *board, Colour c, PieceType pt) {
    return board->pieces[c][pt];
}

inline Bitboard getOccupiedSquares(const ChessBoard *board) {
    return board->pieces[WHITE][ALL_PIECES] | board->pieces[BLACK][ALL_PIECES];
}

inline Square getEnPassantSquare(const ChessBoard *board) {
    return board->enPassant;
}

inline CastlingRights getCastlingRights(const ChessBoard *board) {
    return board->castlingRights;
}

void parseFEN(ChessBoard *board, const char *fenString);
void addPiece(ChessBoard *board, Colour c, PieceType pt, Square sq);

// Assumes that the piece is moving to an EMPTY square
void movePiece(ChessBoard *board, Colour c, PieceType pt, Square fromSquare, Square toSquare); 

void removePiece(ChessBoard *board, Colour c, PieceType pt, Square sq);
void makeMove(ChessBoard *board, const Move *move, IrreversibleBoardState *ibs);
void undoMove(ChessBoard *board, const Move *move, const IrreversibleBoardState *ibs);

bool isSquareAttacked(const ChessBoard *board, Square sq, Colour attackedSide);

void printBitboard(Bitboard b);

#endif