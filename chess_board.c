#include <stdio.h>
#include "chess_board.h"
#include "utility.h"
#include "move_generator.h"

void parseFEN(ChessBoard *board, const char *fenString) {
    static const Colour CHAR_TO_COLOUR[128] = {
        ['P'] = WHITE, ['p'] = BLACK, 
        ['N'] = WHITE, ['n'] = BLACK, 
        ['B'] = WHITE, ['b'] = BLACK, 
        ['R'] = WHITE, ['r'] = BLACK, 
        ['Q'] = WHITE, ['q'] = BLACK, 
        ['K'] = WHITE, ['k'] = BLACK,
        ['w'] = WHITE
    };
    static const PieceType CHAR_TO_PIECE_TYPE[128] = {
        ['P'] = PAWN  , ['p'] = PAWN  , 
        ['N'] = KNIGHT, ['n'] = KNIGHT, 
        ['B'] = BISHOP, ['b'] = BISHOP, 
        ['R'] = ROOK  , ['r'] = ROOK  , 
        ['Q'] = QUEEN , ['q'] = QUEEN , 
        ['K'] = KING  , ['k'] = KING
    };

    Square sq = A8;
    while (*fenString != ' ') {
        unsigned char ch = *fenString++;
        if (ch > 'A') {
            addPiece(board, CHAR_TO_COLOUR[ch], CHAR_TO_PIECE_TYPE[ch], sq++);
        } else if (ch > '/') {
            sq += ch - '0';
        }
    }

    board->sideToMove = CHAR_TO_COLOUR[(unsigned char)*++fenString];
}

void addPiece(ChessBoard *board, Colour c, PieceType pt, Square sq) {
    Bitboard sqBB = squareToBitboard(sq);
    board->pieceTypes[sq] = pt;
    board->pieces[c][ALL_PIECES] |= sqBB;
    board->pieces[c][pt] |= sqBB;
}

void printBitboard(Bitboard b) {
    printf("_________________________________\n");
    for (Bitboard i = 1ULL << 63; i > 0; i >>= 1) {
        if (i & FILE_A_BB) printf("| ");
        if (b & i) printf("1 | ");
        else printf("0 | ");
        if (i & FILE_H_BB) printf("\n_________________________________\n");
    }
}