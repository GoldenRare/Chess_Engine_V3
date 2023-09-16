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


    board->enPassant = NO_SQUARE;
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

void makeMove(ChessBoard *board, const Move *move) {
    Square fromSquare = getFromSquare(move);
    Square toSquare = getToSquare(move);
    MoveType moveType = getMoveType(move);
    Colour stm = board->sideToMove;
    Direction pawnPush = stm ? SOUTH : NORTH;
    PieceType movingPiece = board->pieceTypes[fromSquare];
    PieceType capturedPiece = moveType == EN_PASSANT_CAPTURE ? PAWN : board->pieceTypes[toSquare];
    Square captureSquare = moveType == EN_PASSANT_CAPTURE ? moveSquareInDirection(toSquare, -pawnPush) : toSquare;
    
    if (moveType & CAPTURE) removePiece(board, stm ^ 1, capturedPiece, captureSquare);
    if (moveType & PROMOTION) {
        addPiece(board, stm, KNIGHT + (moveType & 0b11), toSquare);
        removePiece(board, stm, PAWN, fromSquare);
    } else {
        movePiece(board, stm, movingPiece, fromSquare, toSquare);
    }
    board->enPassant = moveType == DOUBLE_PAWN_PUSH ? moveSquareInDirection(toSquare, -pawnPush) : NO_SQUARE;
    board->sideToMove ^= 1;
    
}

bool isSquareAttacked(const ChessBoard *board, Square sq, Colour attackedSide) {
    Colour enemy = attackedSide ^ 1;
    Bitboard occupied = getOccupiedSquares(board);
    Bitboard enemyPawns = board->pieces[enemy][PAWN];
    Bitboard enemyKnights = board->pieces[enemy][KNIGHT];
    Bitboard enemyBishopsQueens = board->pieces[enemy][BISHOP] | board->pieces[enemy][QUEEN];
    Bitboard enemyRooksQueens = board->pieces[enemy][ROOK] | board->pieces[enemy][QUEEN];
    Bitboard enemyKing = board->pieces[enemy][KING];
    return (getPawnAttacks(attackedSide, sq) & enemyPawns) 
         | (getNonSlidingAttacks(KNIGHT_ATTACKER, sq) & enemyKnights)
         | (getSlidingAttacks(occupied, sq, BISHOP_INDEX) & enemyBishopsQueens)
         | (getSlidingAttacks(occupied, sq, ROOK_INDEX) & enemyRooksQueens)
         | (getNonSlidingAttacks(KING_ATTACKER, sq) & enemyKing);
}

/*bool isSquareAttacked(const ChessBoard *board, Square sq, Colour attackedSide) {
    Colour enemy = attackedSide ^ 1;
    if (pawnAttacks[attackedSide][sq] & board->pieces2[enemy][PAWN]) return true;
    if (pieceAttacks[KNIGHT][sq] & board->pieces2[enemy][KNIGHT]) return true;
    if (getSlidingAttacks(board->occupied, sq, BISHOP_INDEX) & (board->pieces2[enemy][BISHOP] | board->pieces2[enemy][QUEEN])) return true;
    if (getSlidingAttacks(board->occupied, sq, ROOK_INDEX) & (board->pieces2[enemy][ROOK] | board->pieces2[enemy][QUEEN])) return true;
    if (pieceAttacks[KING][sq] & board->pieces2[enemy][KING]) return true;
    return false;
}*/

void printBitboard(Bitboard b) {
    printf("_________________________________\n");
    for (Bitboard i = 1ULL << 63; i > 0; i >>= 1) {
        if (i & FILE_A_BB) printf("| ");
        if (b & i) printf("1 | ");
        else printf("0 | ");
        if (i & FILE_H_BB) printf("\n_________________________________\n");
    }
}