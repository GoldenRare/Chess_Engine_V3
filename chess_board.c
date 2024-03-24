#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "chess_board.h"
#include "utility.h"
#include "move_generator.h"
#include "zobrist.h"

Bitboard fullLine[SQUARES][SQUARES];
Bitboard inBetweenLine[SQUARES][SQUARES];
const char *SQUARE_NAME[] = {
        "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8", 
        "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", 
        "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", 
        "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", 
        "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", 
        "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", 
        "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", 
        "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"
};

const char PROMOTION_NAME[] = {'n', 'b', 'r', 'q'};

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
        int sq1Rank = squareToRank(sq1);
        int sq1File = squareToFile(sq1);
        Bitboard sq1BB = squareToBitboard(sq1);
        Bitboard sq1BishopAttacks = getSlidingAttacks(0, sq1, BISHOP_INDEX);
        Bitboard rankBB = rankBitboardOfSquare(sq1);
        Bitboard fileBB = fileBitboardOfSquare(sq1);
        
        for (Square sq2 = sq1 + 1; sq2 < SQUARES; sq2++) {
            Bitboard sq2BB = squareToBitboard(sq2);
            int rankDistance = abs((int) squareToRank(sq2) - sq1Rank);
            int fileDistance = abs((int) squareToFile(sq2) - sq1File);
            Bitboard line = 0;
            Bitboard inBetween = 0;
            if (rankDistance == 0 || fileDistance == 0) {
                line = rankDistance == 0 ? rankBB : fileBB;
                inBetween = (getSlidingAttacks(sq2BB, sq1, ROOK_INDEX) & getSlidingAttacks(sq1BB, sq2, ROOK_INDEX)) | sq1BB | sq2BB;
            } else if (rankDistance == fileDistance) {
                line = (sq1BishopAttacks & getSlidingAttacks(0, sq2, BISHOP_INDEX)) | sq1BB | sq2BB;
                inBetween = (getSlidingAttacks(sq2BB, sq1, BISHOP_INDEX) & getSlidingAttacks(sq1BB, sq2, BISHOP_INDEX)) | sq1BB | sq2BB;  
            }
            fullLine[sq1][sq2] = line;
            fullLine[sq2][sq1] = line;
            inBetweenLine[sq1][sq2] = inBetween;
            inBetweenLine[sq2][sq1] = inBetween;
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
            board->positionKey ^= zobristHashes.pieceOnSquare[CHAR_TO_PIECE_TYPE[ch] + COLOUR_OFFSET * CHAR_TO_COLOUR[ch]][sq];
        } else if (ch > '/') {
            sq += ch - '0';
        }
    }

    /* 2) Side to Move */
    board->sideToMove = CHAR_TO_COLOUR[(unsigned char)*++fenString];
    if (board->sideToMove) board->positionKey ^= zobristHashes.sideToMove;
    fenString++;

    /* 3) Castling Ability */
    while (*++fenString != ' ') board->castlingRights |= CHAR_TO_CASTLING_RIGHTS[(unsigned char) *fenString];
    board->positionKey ^= zobristHashes.castlingRights[board->castlingRights];
    
    /* 4) En Passant Target Square */
    if (*++fenString != '-') {
            int file = *fenString - 'a';
            int rank = *++fenString - '8';
            board->enPassant = rank * -8 + file;
            board->positionKey ^= zobristHashes.enPassant[file];
    } else {
        board->enPassant = NO_SQUARE;
    }

    /* 5) Miscellaneous Data */
    board->checkers = attackersTo(board, getKingSquare(board, board->sideToMove), board->sideToMove, getOccupiedSquares(board));
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
    ibs->positionKey = board->positionKey;
    ibs->checkers = board->checkers;
    
    if (moveType & CAPTURE) {
        Square captureSquare = moveType == EN_PASSANT_CAPTURE ? moveSquareInDirection(toSquare, stm ? NORTH : SOUTH) : toSquare;
        ibs->capturedPiece = board->pieceTypes[captureSquare];
        removePiece(board, stm ^ 1, ibs->capturedPiece, captureSquare);
        board->positionKey ^= zobristHashes.pieceOnSquare[ibs->capturedPiece + COLOUR_OFFSET * (stm ^ 1)][captureSquare];
    } else if (moveType == KINGSIDE_CASTLE || moveType == QUEENSIDE_CASTLE) {
        bool isQueenSideCastle = moveType & 1;
        Square rookFromSquare = isQueenSideCastle ? moveSquareInDirection(toSquare, WEST + WEST) : moveSquareInDirection(toSquare, EAST);
        Square rookToSquare   = isQueenSideCastle ? moveSquareInDirection(fromSquare, WEST)      : moveSquareInDirection(fromSquare, EAST);
        movePiece(board, stm, ROOK, rookFromSquare, rookToSquare);
        board->positionKey ^= zobristHashes.pieceOnSquare[ROOK + COLOUR_OFFSET * stm][rookFromSquare] 
                           ^  zobristHashes.pieceOnSquare[ROOK + COLOUR_OFFSET * stm][rookToSquare  ];
    } 
    
    if (moveType & PROMOTION) {
        PieceType pt = KNIGHT + (moveType & PROMOTION_PIECE_OFFSET_MASK);
        addPiece(board, stm, pt, toSquare);
        board->positionKey ^= zobristHashes.pieceOnSquare[pt + COLOUR_OFFSET * stm][toSquare];
        removePiece(board, stm, PAWN, fromSquare);
        board->positionKey ^= zobristHashes.pieceOnSquare[PAWN + COLOUR_OFFSET * stm][fromSquare];
    } else {
        movePiece(board, stm, board->pieceTypes[fromSquare], fromSquare, toSquare);
        board->positionKey ^= zobristHashes.pieceOnSquare[board->pieceTypes[fromSquare] + COLOUR_OFFSET * stm][fromSquare] 
                           ^  zobristHashes.pieceOnSquare[board->pieceTypes[fromSquare] + COLOUR_OFFSET * stm][toSquare  ];
    }

    ibs->castlingRights = board->castlingRights;
    ibs->enPassant = board->enPassant;
    if (board->enPassant != NO_SQUARE) board->positionKey ^= zobristHashes.enPassant[squareToFile(board->enPassant)];
    if (board->castlingRights) {
        board->positionKey ^= zobristHashes.castlingRights[board->castlingRights];
        board->castlingRights &= CASTLING_RIGHTS_MASK[fromSquare] & CASTLING_RIGHTS_MASK[toSquare];
        board->positionKey ^= zobristHashes.castlingRights[board->castlingRights];
    } 
    if (moveType == DOUBLE_PAWN_PUSH) {
        board->enPassant = moveSquareInDirection(toSquare, stm ? NORTH : SOUTH);
        board->positionKey ^= zobristHashes.enPassant[squareToFile(board->enPassant)];
    } else {
        board->enPassant = NO_SQUARE;
    }
    board->sideToMove ^= 1;
    board->positionKey ^= zobristHashes.sideToMove;
    board->checkers = attackersTo(board, getKingSquare(board, board->sideToMove), board->sideToMove, getOccupiedSquares(board));
}

void undoMove(ChessBoard *board, const Move *move, const IrreversibleBoardState *ibs) {
    board->positionKey = ibs->positionKey;
    board->sideToMove ^= 1;
    board->enPassant = ibs->enPassant;
    board->castlingRights = ibs->castlingRights;
    board->checkers = ibs->checkers;
    
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

bool isLegalMove(const ChessBoard *board, const Move *move, Bitboard pinned) {
    Square fromSquare = getFromSquare(move);
    Square toSquare = getToSquare(move);
    MoveType moveType = getMoveType(move);
    Colour stm = board->sideToMove;

    if (moveType == KINGSIDE_CASTLE || moveType == QUEENSIDE_CASTLE) {
        Direction towardsKing = moveType & 1 ? EAST : WEST;
        Bitboard occupied = getOccupiedSquares(board);
        while (toSquare != fromSquare) {
            if (attackersTo(board, toSquare, stm, occupied)) return false;
            toSquare = moveSquareInDirection(toSquare, towardsKing);
        }
        return true;
    }

    Square kingSquare = getKingSquare(board, stm);
    Bitboard fromSquareBB = squareToBitboard(fromSquare);
    if (moveType == EN_PASSANT_CAPTURE) {
        Bitboard occupied = getOccupiedSquares(board) ^ fromSquareBB ^ squareToBitboard(toSquare) 
                          ^ squareToBitboard(moveSquareInDirection(toSquare, stm ? NORTH : SOUTH));
        Colour enemy = stm ^ 1;
        Bitboard queens = getPieces(board, enemy, QUEEN);
        return !((getSlidingAttacks(occupied, kingSquare, BISHOP_INDEX) & (getPieces(board, enemy, BISHOP) | queens))
               | (getSlidingAttacks(occupied, kingSquare, ROOK_INDEX  ) & (getPieces(board, enemy, ROOK  ) | queens)));
    }
    
    if (fromSquare == kingSquare) {
        return !attackersTo(board, toSquare, stm, getOccupiedSquares(board) ^ fromSquareBB);
    }
    
    return !(pinned & fromSquareBB) || fullLine[fromSquare][toSquare] & squareToBitboard(kingSquare);
}

// TODO: Operation is currently expensive
bool isPseudoMove(const ChessBoard *board, Move move) {
    MoveObject moveList[256];
    MoveObject *startList = moveList;
    MoveObject *endList = createMoveList(board, moveList);
    while (startList < endList) {
        if (startList->move == move) return true;
        startList++;
    }
    return false;
}

Bitboard getPinnedPieces(const ChessBoard *board) {
    Colour stm = board->sideToMove;
    Colour enemy = stm ^ 1;
    Square kingSq = getKingSquare(board, stm);
    Bitboard enemyQueens = getPieces(board, enemy, QUEEN);
    Bitboard stmPieces = getPieces(board, stm, ALL_PIECES);
    Bitboard occupiedSquares = stmPieces | getPieces(board, enemy, ALL_PIECES);

    Bitboard attacks = getSlidingAttacks(occupiedSquares, kingSq, ROOK_INDEX);
    Bitboard potentiallyPinned = attacks & stmPieces;
    Bitboard pinners = (getSlidingAttacks(occupiedSquares ^ potentiallyPinned, kingSq, ROOK_INDEX) ^ attacks) & (getPieces(board, enemy, ROOK) | enemyQueens);

    Bitboard pinned = 0ULL;
    while (pinners) {
        pinned |= inBetweenLine[bitboardToSquareWithReset(&pinners)][kingSq] & potentiallyPinned;
    }

    attacks = getSlidingAttacks(occupiedSquares, kingSq, BISHOP_INDEX);
    potentiallyPinned = attacks & stmPieces;
    pinners = (getSlidingAttacks(occupiedSquares ^ potentiallyPinned, kingSq, BISHOP_INDEX) ^ attacks) & (getPieces(board, enemy, BISHOP) | enemyQueens);
    while (pinners) {
        pinned |= inBetweenLine[bitboardToSquareWithReset(&pinners)][kingSq] & potentiallyPinned;
    }

    return pinned;
}

Bitboard attackersTo(const ChessBoard *board, Square sq, Colour attackedSide, Bitboard occupied) {
    Colour enemy = attackedSide ^ 1;
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