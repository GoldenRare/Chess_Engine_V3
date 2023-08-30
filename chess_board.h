#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include <stdbool.h>
#include "utility.h"

typedef struct ChessBoard {
    Colour sideToMove;
    PieceType pieceTypes[SQUARES];
    Bitboard piecesOnSide[COLOURS];
    Bitboard pieces[COLOURS][PIECE_TYPES];
    Bitboard occupied;
    Bitboard empty;
} ChessBoard;

inline Colour getSideToMove(const ChessBoard *board) {
    return board->sideToMove;
}

inline Bitboard getPieces(const ChessBoard *board, Colour c, PieceType pt) {
    return board->pieces[c][pt];
}

inline Bitboard getPiecesOnSide(const ChessBoard *board, Colour c) {
    return board->piecesOnSide[c];
}

void parseFEN(ChessBoard *board, const char *fenString);
void addPiece(ChessBoard *board, Colour c, PieceType pt, Square sq);
void movePiece(ChessBoard *board, Square fromSquare, Square toSquare);
void makeMove(ChessBoard *board, const Move *move);

bool isSquareAttacked(const ChessBoard *board, Square sq, Colour attackedSide);

void printBitboard(Bitboard b);

#endif