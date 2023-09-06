#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include <stdbool.h>
#include "utility.h"

typedef struct ChessBoard {
    Colour sideToMove;
    PieceType pieceTypes[SQUARES];
    Bitboard pieces[COLOURS][PIECE_TYPES];
} ChessBoard;

inline Colour getSideToMove(const ChessBoard *board) {
    return board->sideToMove;
}

inline Bitboard getPieces(const ChessBoard *board, Colour c, PieceType pt) {
    return board->pieces[c][pt];
}

inline Bitboard getPiecesOnSide(const ChessBoard *board, Colour c) {
    return board->pieces[c][ALL_PIECES];
}

inline Bitboard getOccupiedSquares(const ChessBoard *board) {
    return board->pieces[WHITE][ALL_PIECES] | board->pieces[BLACK][ALL_PIECES];
}

void parseFEN(ChessBoard *board, const char *fenString);
void addPiece(ChessBoard *board, Colour c, PieceType pt, Square sq);

// Assumes that the piece is moving to an EMPTY square
void movePiece(ChessBoard *board, Colour c, PieceType pt, Square fromSquare, Square toSquare); 

void removePiece(ChessBoard *board, Colour c, PieceType pt, Square sq);
void makeMove(ChessBoard *board, const Move *move);

bool isSquareAttacked(const ChessBoard *board, Square sq, Colour attackedSide);

void printBitboard(Bitboard b);

#endif