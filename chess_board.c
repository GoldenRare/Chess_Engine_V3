#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "chess_board.h"
#include "utility.h"
#include "move_generator.h"

Bitboard fullLine[SQUARES][SQUARES];

// Bit representation: 1s represent the castling rights to stay on, 0s represent the castling rights to turn off
static const CastlingRights CASTLING_RIGHTS_MASK[SQUARES] = {
    [A1] = BLACK_RIGHTS | WHITE_KINGSIDE, [B1] = ALL_RIGHTS, [C1] = ALL_RIGHTS, [D1] = ALL_RIGHTS, [E1] = BLACK_RIGHTS, [F1] = ALL_RIGHTS, [G1] = ALL_RIGHTS, [H1] = BLACK_RIGHTS | WHITE_QUEENSIDE,
    [A2] = ALL_RIGHTS,                    [B2] = ALL_RIGHTS, [C2] = ALL_RIGHTS, [D2] = ALL_RIGHTS, [E2] = ALL_RIGHTS,   [F2] = ALL_RIGHTS, [G2] = ALL_RIGHTS, [H2] = ALL_RIGHTS,
    [A3] = ALL_RIGHTS,                    [B3] = ALL_RIGHTS, [C3] = ALL_RIGHTS, [D3] = ALL_RIGHTS, [E3] = ALL_RIGHTS,   [F3] = ALL_RIGHTS, [G3] = ALL_RIGHTS, [H3] = ALL_RIGHTS,
    [A4] = ALL_RIGHTS,                    [B4] = ALL_RIGHTS, [C4] = ALL_RIGHTS, [D4] = ALL_RIGHTS, [E4] = ALL_RIGHTS,   [F4] = ALL_RIGHTS, [G4] = ALL_RIGHTS, [H4] = ALL_RIGHTS,
    [A5] = ALL_RIGHTS,                    [B5] = ALL_RIGHTS, [C5] = ALL_RIGHTS, [D5] = ALL_RIGHTS, [E5] = ALL_RIGHTS,   [F5] = ALL_RIGHTS, [G5] = ALL_RIGHTS, [H5] = ALL_RIGHTS,
    [A6] = ALL_RIGHTS,                    [B6] = ALL_RIGHTS, [C6] = ALL_RIGHTS, [D6] = ALL_RIGHTS, [E6] = ALL_RIGHTS,   [F6] = ALL_RIGHTS, [G6] = ALL_RIGHTS, [H6] = ALL_RIGHTS,
    [A7] = ALL_RIGHTS,                    [B7] = ALL_RIGHTS, [C7] = ALL_RIGHTS, [D7] = ALL_RIGHTS, [E7] = ALL_RIGHTS,   [F7] = ALL_RIGHTS, [G7] = ALL_RIGHTS, [H7] = ALL_RIGHTS,
    [A8] = BLACK_KINGSIDE | WHITE_RIGHTS, [B8] = ALL_RIGHTS, [C8] = ALL_RIGHTS, [D8] = ALL_RIGHTS, [E8] = WHITE_RIGHTS, [F8] = ALL_RIGHTS, [G8] = ALL_RIGHTS, [H8] = BLACK_QUEENSIDE | WHITE_RIGHTS,
};

void initializeChessBoard() {
    for (Square sq1 = 0; sq1 < SQUARES; sq1++) {
        int rank1 = squareToRank(sq1);
        int file1 = squareToFile(sq1);
        Bitboard rankBB = rankBitboardOfSquare(sq1);
        Bitboard fileBB = fileBitboardOfSquare(sq1);
        Bitboard sq1BB = squareToBitboard(sq1);
        Bitboard sq1BishopAttacks = getSlidingAttacks(0, sq1, BISHOP_INDEX);
        
        for (Square sq2 = sq1 + 1; sq2 < SQUARES; sq2++) {
            int rankDistance = abs((int) squareToRank(sq2) - rank1);
            int fileDistance = abs((int) squareToFile(sq2) - file1);
            Bitboard line = 0;
            if (rankDistance == 0) {
                line = rankBB;
            } else if (fileDistance == 0) {
                line = fileBB;
            } else if (rankDistance == fileDistance) {
                line = (sq1BishopAttacks & getSlidingAttacks(0, sq2, BISHOP_INDEX))
                      | sq1BB | squareToBitboard(sq2);  
            }
            fullLine[sq1][sq2] = line;
            fullLine[sq2][sq1] = line;
        }
    }
}

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
    static const CastlingRights CHAR_TO_CASTLING_RIGHTS[128] = {
        ['Q'] = WHITE_QUEENSIDE, ['q'] = BLACK_QUEENSIDE, 
        ['K'] = WHITE_KINGSIDE , ['k'] = BLACK_KINGSIDE,
        ['-'] = 0
    };

    /* 1) Piece Placement */
    Square sq = A8;
    while (*fenString != ' ') {
        unsigned char ch = *fenString++;
        if (ch > 'A') {
            addPiece(board, CHAR_TO_COLOUR[ch], CHAR_TO_PIECE_TYPE[ch], sq++);
        } else if (ch > '/') {
            sq += ch - '0';
        }
    }

    /* 2) Side to Move */
    board->sideToMove = CHAR_TO_COLOUR[(unsigned char)*++fenString];
    fenString++;

    /* 3) Castling Ability */
    while (*++fenString != ' ') board->castlingRights |= CHAR_TO_CASTLING_RIGHTS[(unsigned char) *fenString];
    
    /* 4) En Passant Target Square */
    if (*++fenString != '-') {
            int file = *fenString - 'a';
            int rank = *++fenString - '8';
            board->enPassant = rank * -8 + file;
    } else {
        board->enPassant = NO_SQUARE;
    }
}

void addPiece(ChessBoard *board, Colour c, PieceType pt, Square sq) {
    Bitboard sqBB = squareToBitboard(sq);
    board->pieceTypes[sq] = pt;
    board->pieces[c][pt] |= sqBB;
    board->pieces[c][ALL_PIECES] |= sqBB;
}

void movePiece(ChessBoard *board, Colour c, PieceType pt, Square fromSquare, Square toSquare) {
    Bitboard fromToBB = squareToBitboard(fromSquare) | squareToBitboard(toSquare);
    board->pieceTypes[toSquare] = pt;
    board->pieceTypes[fromSquare] = NO_PIECE;
    board->pieces[c][pt] ^= fromToBB;
    board->pieces[c][ALL_PIECES] ^= fromToBB;
}

void removePiece(ChessBoard *board, Colour c, PieceType pt, Square sq) {
    Bitboard sqBB = squareToBitboard(sq);
    board->pieceTypes[sq] = NO_PIECE;
    board->pieces[c][pt] ^= sqBB;
    board->pieces[c][ALL_PIECES] ^= sqBB;
}

void makeMove(ChessBoard *board, const Move *move, IrreversibleBoardState *ibs) {
    Square fromSquare = getFromSquare(move);
    Square toSquare = getToSquare(move);
    MoveType moveType = getMoveType(move);
    Colour stm = board->sideToMove;
    
    if (moveType & CAPTURE) {
        bool isEnPassantCapture = moveType == EN_PASSANT_CAPTURE;
        PieceType capturedPiece = isEnPassantCapture ? PAWN : board->pieceTypes[toSquare];
        Square captureSquare = isEnPassantCapture ? moveSquareInDirection(toSquare, stm ? NORTH : SOUTH) : toSquare;
        ibs->capturedPiece = capturedPiece;
        removePiece(board, stm ^ 1, capturedPiece, captureSquare);
    } else if (moveType == KINGSIDE_CASTLE || moveType == QUEENSIDE_CASTLE) {
        bool isQueenSideCastle = moveType & 1;
        Square rookFromSquare = isQueenSideCastle ? moveSquareInDirection(toSquare, WEST + WEST) : moveSquareInDirection(toSquare, EAST);
        Square rookToSquare   = isQueenSideCastle ? moveSquareInDirection(fromSquare, WEST)      : moveSquareInDirection(fromSquare, EAST);
        movePiece(board, stm, ROOK, rookFromSquare, rookToSquare);
    } 
    
    if (moveType & PROMOTION) {
        addPiece(board, stm, KNIGHT + (moveType & PROMOTION_PIECE_OFFSET_MASK), toSquare);
        removePiece(board, stm, PAWN, fromSquare);
    } else {
        movePiece(board, stm, board->pieceTypes[fromSquare], fromSquare, toSquare);
    }

    ibs->castlingRights = board->castlingRights;
    ibs->enPassant = board->enPassant;
    if (board->castlingRights) board->castlingRights &= CASTLING_RIGHTS_MASK[fromSquare] & CASTLING_RIGHTS_MASK[toSquare];
    board->enPassant = moveType == DOUBLE_PAWN_PUSH ? moveSquareInDirection(toSquare, stm ? NORTH : SOUTH) : NO_SQUARE;
    board->sideToMove ^= 1;
}

void undoMove(ChessBoard *board, const Move *move, const IrreversibleBoardState *ibs) {
    board->sideToMove ^= 1;
    board->enPassant = ibs->enPassant;
    board->castlingRights = ibs->castlingRights;
    
    Square fromSquare = getFromSquare(move);
    Square toSquare = getToSquare(move);
    MoveType moveType = getMoveType(move);
    Colour stm = board->sideToMove;
    
    if (moveType & PROMOTION) {
        removePiece(board, stm, board->pieceTypes[toSquare], toSquare);
        addPiece(board, stm, PAWN, fromSquare);
    } else {
        movePiece(board, stm, board->pieceTypes[toSquare], toSquare, fromSquare);
    }

    if (moveType & CAPTURE) {
        bool isEnPassantCapture = moveType == EN_PASSANT_CAPTURE;
        PieceType capturedPiece = ibs->capturedPiece;
        Square captureSquare = isEnPassantCapture ? moveSquareInDirection(toSquare, stm ? NORTH : SOUTH) : toSquare;
        addPiece(board, stm ^ 1, capturedPiece, captureSquare);
    } else if (moveType == KINGSIDE_CASTLE || moveType == QUEENSIDE_CASTLE) {
        bool isQueenSideCastle = moveType & 1;
        Square rookFromSquare = isQueenSideCastle ? moveSquareInDirection(toSquare, WEST + WEST) : moveSquareInDirection(toSquare, EAST);
        Square rookToSquare   = isQueenSideCastle ? moveSquareInDirection(fromSquare, WEST)      : moveSquareInDirection(fromSquare, EAST);
        movePiece(board, stm, ROOK, rookToSquare, rookFromSquare);
    } 
}

/*bool isLegalMove(const ChessBoard *board, const Move *move) {
    Square fromSquare = getFromSquare(move);
    Square toSquare = getToSquare(move);
    MoveType moveType = getMoveType(move);
    Colour stm = board->sideToMove;

    if (moveType == EN_PASSANT_CAPTURE) {
        //fullLine[fromSquare][ksq]
    }

    if (board->pinned & squareToBitboard(fromSquare)) {
        return fullLine[fromSquare][toSquare] & board->pieces[stm][KING];
    }
}*/

bool isSquareAttacked(const ChessBoard *board, Square sq, Colour attackedSide) {
    Colour enemy = attackedSide ^ 1;
    Bitboard occupied = getOccupiedSquares(board);
    Bitboard queens = getPieces(board, enemy, QUEEN);
    return (getPawnAttacks(attackedSide, sq)              &  getPieces(board, enemy, PAWN))
         | (getNonSlidingAttacks(KNIGHT_ATTACKER, sq)     &  getPieces(board, enemy, KNIGHT))
         | (getSlidingAttacks(occupied, sq, BISHOP_INDEX) & (getPieces(board, enemy, BISHOP) | queens))
         | (getSlidingAttacks(occupied, sq, ROOK_INDEX)   & (getPieces(board, enemy, ROOK)   | queens))
         | (getNonSlidingAttacks(KING_ATTACKER, sq)       &  getPieces(board, enemy, KING));
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