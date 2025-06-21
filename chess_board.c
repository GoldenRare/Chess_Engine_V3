#include <stdlib.h>
#include <stdint.h>
#include "chess_board.h"
#include "utility.h"
#include "move_generator.h"
#include "nnue.h"

typedef struct Zobrist {
    uint64_t pieceOnSquare[PIECE_TYPES * COLOURS][SQUARES]; // TODO: Some unnecessary space due to NO_PIECE
    uint64_t castlingRights[ALL_RIGHTS + 1]; // +1 to include ALL_RIGHTS
    uint64_t enPassant[FILES];
    uint64_t sideToMove;
} Zobrist;

static Zobrist zobristHashes;

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

static void initializeZobrist() {
    uint64_t seed = 1070372;
    for (PieceType pt = PAWN; pt < PIECE_TYPES; pt++) {
        for (Square sq = 0; sq < SQUARES; sq++) {
            zobristHashes.pieceOnSquare[pt][sq] = random64BitNumber(&seed);
            zobristHashes.pieceOnSquare[pt + COLOUR_OFFSET][sq] = random64BitNumber(&seed);
        }
    }

    for (CastlingRights cr = 0; cr < ALL_RIGHTS + 1; cr++) {
        zobristHashes.castlingRights[cr] = random64BitNumber(&seed);
    }

    for (File file = FILE_A; file < FILES; file++) {
        zobristHashes.enPassant[file] = random64BitNumber(&seed);
    }

    zobristHashes.sideToMove = random64BitNumber(&seed);
}

// TODO: Make inline after accumulator optimizations are done
static void addPiece(ChessBoard *restrict board, Colour c, PieceType pt, Square sq) {
    Bitboard sqBB = squareToBitboard(sq);
    board->pieceTypes[sq] = pt;
    board->pieces[c][pt] |= sqBB;
    board->pieces[c][ALL_PIECES] |= sqBB;
    accumulatorAdd(c, pt, sq);
}

// TODO: Make inline after accumulator optimizations are done
static void movePiece(ChessBoard *restrict board, Colour c, PieceType pt, Square fromSquare, Square toSquare) {
    Bitboard fromToBB = squareToBitboard(fromSquare) | squareToBitboard(toSquare);
    board->pieceTypes[toSquare] = pt;
    board->pieceTypes[fromSquare] = NO_PIECE;
    board->pieces[c][pt] ^= fromToBB;
    board->pieces[c][ALL_PIECES] ^= fromToBB;
    accumulatorAddSub(c, pt, fromSquare, toSquare);
}

// TODO: Make inline after accumulator optimizations are done
static void removePiece(ChessBoard *restrict board, Colour c, PieceType pt, Square sq) {
    Bitboard sqBB = squareToBitboard(sq);
    board->pieceTypes[sq] = NO_PIECE;
    board->pieces[c][pt] ^= sqBB;
    board->pieces[c][ALL_PIECES] ^= sqBB;
    accumulatorSub(c, pt, sq);
}

static Bitboard getPinnedPieces(const ChessBoard *restrict board) {
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

static Bitboard attackersTo(const ChessBoard *restrict board, Square sq, Colour attackedSide, Bitboard occupied) {
    Colour enemy = attackedSide ^ 1;
    Bitboard queens = getPieces(board, enemy, QUEEN);
    return (getPawnAttacks(attackedSide, sq)              &  getPieces(board, enemy, PAWN))
         | (getNonSlidingAttacks(KNIGHT_ATTACKER, sq)     &  getPieces(board, enemy, KNIGHT))
         | (getSlidingAttacks(occupied, sq, BISHOP_INDEX) & (getPieces(board, enemy, BISHOP) | queens))
         | (getSlidingAttacks(occupied, sq, ROOK_INDEX)   & (getPieces(board, enemy, ROOK)   | queens))
         | (getNonSlidingAttacks(KING_ATTACKER, sq)       &  getPieces(board, enemy, KING));
}

void initializeChessBoard() {
    initializeZobrist();
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
            Bitboard inBetween = sq1BB | sq2BB;
            if (rankDistance == 0 || fileDistance == 0) {
                line = rankDistance == 0 ? rankBB : fileBB;
                inBetween |= (getSlidingAttacks(sq2BB, sq1, ROOK_INDEX) & getSlidingAttacks(sq1BB, sq2, ROOK_INDEX));
            } else if (rankDistance == fileDistance) {
                line = (sq1BishopAttacks & getSlidingAttacks(0, sq2, BISHOP_INDEX)) | sq1BB | sq2BB;
                inBetween |= (getSlidingAttacks(sq2BB, sq1, BISHOP_INDEX) & getSlidingAttacks(sq1BB, sq2, BISHOP_INDEX));  
            }
            fullLine[sq1][sq2] = line;
            fullLine[sq2][sq1] = line;
            inBetweenLine[sq1][sq2] = inBetween;
            inBetweenLine[sq2][sq1] = inBetween;
        }
    }
}

void parseFEN(ChessBoard *board, ChessBoardHistory *history, const char *restrict fen) {
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

    board->history = history;
    /* 1) Piece Placement */
    Square sq = A8;
    while (*fen != ' ') {
        unsigned char ch = *fen++;
        if (ch > 'A') {
            addPiece(board, CHAR_TO_COLOUR[ch], CHAR_TO_PIECE_TYPE[ch], sq++);
            board->history->positionKey ^= zobristHashes.pieceOnSquare[CHAR_TO_PIECE_TYPE[ch] + COLOUR_OFFSET * CHAR_TO_COLOUR[ch]][sq];
        } else if (ch > '/') {
            sq += ch - '0';
        }
    }

    /* 2) Side to Move */
    board->sideToMove = CHAR_TO_COLOUR[(unsigned char)*++fen];
    if (board->sideToMove) board->history->positionKey ^= zobristHashes.sideToMove;
    fen++;

    /* 3) Castling Ability */
    while (*++fen != ' ') board->history->castlingRights |= CHAR_TO_CASTLING_RIGHTS[(unsigned char) *fen];
    board->history->positionKey ^= zobristHashes.castlingRights[board->history->castlingRights];
    
    /* 4) En Passant Target Square */
    if (*++fen != '-') {
            int file = *fen - 'a';
            int rank = *++fen - '8';
            board->history->enPassant = rank * -8 + file;
            board->history->positionKey ^= zobristHashes.enPassant[file];
    } else {
        board->history->enPassant = NO_SQUARE;
    }
    fen++;

    /* 5) Halfmove Clock */
    board->history->halfmoveClock = 0;
    while (*++fen != ' ') board->history->halfmoveClock = board->history->halfmoveClock * 10 + *fen - '0';

    /* 6) Fullmove Counter -> Ply */
    board->ply = 0;
    while (*++fen) board->ply = board->ply * 10 + *fen - '0';
    board->ply = (board->ply << 1) - (board->sideToMove ^ 1);

    /* 7) Miscellaneous Data */
    board->history->checkers = attackersTo(board, getKingSquare(board, board->sideToMove), board->sideToMove, getOccupiedSquares(board));
    board->history->pinnedPieces = getPinnedPieces(board);
}

void getFEN(const ChessBoard *restrict board, char *restrict destination) {
    constexpr char PIECE_TYPE_TO_CHAR[] = " PNBRQK";
    constexpr char CASTLING_RIGHTS_TO_CHAR[] = {
        [WHITE_QUEENSIDE] = 'Q', [BLACK_QUEENSIDE] = 'q', 
        [WHITE_KINGSIDE]  = 'K', [BLACK_KINGSIDE]  = 'k'
    };

    /* 1) Piece Placement */
    int empty = 0;
    for (Square sq = A8; sq < SQUARES; sq++) {
        if (board->pieceTypes[sq]) {
            if (empty) *destination++ = '0' + empty;
            empty = 0;
            char ch = PIECE_TYPE_TO_CHAR[board->pieceTypes[sq]];
            *destination++ = ch + 32 * (bool) (board->pieces[BLACK][board->pieceTypes[sq]] & squareToBitboard(sq));
        } else empty++;

        if (squareToBitboard(sq) & FILE_H_BB) {
            if (empty) *destination++ = '0' + empty;
            empty = 0;
            if (sq != H1) *destination++ = '/';
        }
    }
    *destination++ = ' ';

    /* 2) Side to Move */
    *destination++ = board->sideToMove ? 'b' : 'w';
    *destination++ = ' ';

    /* 3) Castling Ability */
    if (board->history->castlingRights) {
        for (CastlingRights cr = WHITE_KINGSIDE; cr <= BLACK_QUEENSIDE; cr <<= 1)
            if (board->history->castlingRights & cr)
                *destination++ = CASTLING_RIGHTS_TO_CHAR[board->history->castlingRights & cr];
    } else {
        *destination++ = '-';
    }
    *destination++ = ' ';

    /* 4) En Passant Target Square */
    if (board->history->enPassant != NO_SQUARE) {
        *destination++ = SQUARE_NAME[board->history->enPassant][0];
        *destination++ = SQUARE_NAME[board->history->enPassant][1];
    } else {
        *destination++ = '-';
    }
    *destination++ = ' ';

    /* 5) Halfmove Clock */
    int halfmoveClock = board->history->halfmoveClock;
    char helper[8];
    int i = 0;
    helper[i++] = '0' + halfmoveClock % 10;
    halfmoveClock /= 10;
    while (halfmoveClock) {
        helper[i++] = '0' + halfmoveClock % 10;
        halfmoveClock /= 10;
    }
    while (i > 0) *destination++ = helper[--i];
    *destination++ = ' ';

    /* 6) Fullmove Counter -> Ply */
    int fullmoveCounter = board->ply / 2 + (board->sideToMove == WHITE);
    helper[i++] = '0' + fullmoveCounter % 10;
    fullmoveCounter /= 10;
    while (fullmoveCounter) {
        helper[i++] = '0' + fullmoveCounter % 10;
        fullmoveCounter /= 10;
    }
    while (i > 0) *destination++ = helper[--i];

    *destination = '\0';
}

void makeMove(ChessBoard *board, ChessBoardHistory *history, Move move) {
    Square fromSquare = getFromSquare(move);
    Square   toSquare = getToSquare  (move);
    MoveType moveType = getMoveType  (move);
    Square captureSquare = moveType & EN_PASSANT ? moveSquareInDirection(toSquare, board->sideToMove ? NORTH : SOUTH) : toSquare;
    PieceType colOffset = COLOUR_OFFSET * board->sideToMove;

    history->previous = board->history;
    history->positionKey = board->history->positionKey;
    history->capturedPiece = board->pieceTypes[captureSquare];
    history->castlingRights = board->history->castlingRights;
    history->halfmoveClock = board->history->halfmoveClock + 1;

    if (history->capturedPiece) {
        removePiece(board, board->sideToMove ^ 1, history->capturedPiece, captureSquare);
        history->positionKey ^= zobristHashes.pieceOnSquare[history->capturedPiece + COLOUR_OFFSET * (board->sideToMove ^ 1)][captureSquare];
        history->halfmoveClock = 0;
    } else if (moveType == CASTLE) {
        bool isKingSideCastle = toSquare > fromSquare;
        Square rookFromSquare = isKingSideCastle ? moveSquareInDirection(toSquare  , EAST) : moveSquareInDirection(toSquare  , WEST + WEST);
        Square rookToSquare   = isKingSideCastle ? moveSquareInDirection(fromSquare, EAST) : moveSquareInDirection(fromSquare, WEST       );
        movePiece(board, board->sideToMove, ROOK, rookFromSquare, rookToSquare);
        history->positionKey ^= zobristHashes.pieceOnSquare[ROOK + colOffset][rookFromSquare] 
                           ^  zobristHashes.pieceOnSquare[ROOK + colOffset][rookToSquare  ];
    } 
    
    if (board->pieceTypes[fromSquare] == PAWN) history->halfmoveClock = 0;
    if (moveType & PROMOTION) {
        PieceType pt = KNIGHT + (moveType & PROMOTION_PIECE_OFFSET_MASK);
        addPiece(board, board->sideToMove, pt, toSquare);
        history->positionKey ^= zobristHashes.pieceOnSquare[pt + colOffset][toSquare];
        removePiece(board, board->sideToMove, PAWN, fromSquare);
        history->positionKey ^= zobristHashes.pieceOnSquare[PAWN + colOffset][fromSquare];
    } else {
        movePiece(board, board->sideToMove, board->pieceTypes[fromSquare], fromSquare, toSquare);
        history->positionKey ^= zobristHashes.pieceOnSquare[board->pieceTypes[fromSquare] + colOffset][fromSquare] 
                           ^  zobristHashes.pieceOnSquare[board->pieceTypes[fromSquare] + colOffset][toSquare  ];
    }

    if (board->history->enPassant != NO_SQUARE) history->positionKey ^= zobristHashes.enPassant[squareToFile(board->history->enPassant)];
    if (history->castlingRights) {
        history->positionKey ^= zobristHashes.castlingRights[history->castlingRights];
        history->castlingRights &= CASTLING_RIGHTS_MASK[fromSquare] & CASTLING_RIGHTS_MASK[toSquare];
        history->positionKey ^= zobristHashes.castlingRights[history->castlingRights];
    } 
    if (moveType == DOUBLE_PAWN_PUSH) {
        history->enPassant = moveSquareInDirection(toSquare, board->sideToMove ? NORTH : SOUTH);
        history->positionKey ^= zobristHashes.enPassant[squareToFile(history->enPassant)];
    } else {
        history->enPassant = NO_SQUARE;
    }

    board->history = history;
    board->sideToMove ^= 1;
    board->ply++;
    history->positionKey ^= zobristHashes.sideToMove;
    history->checkers = attackersTo(board, getKingSquare(board, board->sideToMove), board->sideToMove, getOccupiedSquares(board));
    history->pinnedPieces = getPinnedPieces(board);
}

void undoMove(ChessBoard *restrict board, Move move) {
    Square fromSquare = getFromSquare(move);
    Square   toSquare = getToSquare  (move);
    MoveType moveType = getMoveType  (move);
    Colour stm = board->sideToMove ^= 1;
    PieceType capturedPiece = board->history->capturedPiece;

    board->history = board->history->previous;
    board->ply--;
    
    if (moveType & PROMOTION) {
        removePiece(board, stm, board->pieceTypes[toSquare], toSquare);
        addPiece(board, stm, PAWN, fromSquare);
    } else {
        movePiece(board, stm, board->pieceTypes[toSquare], toSquare, fromSquare);
    }

    if (capturedPiece) {
        Square captureSquare = moveType & EN_PASSANT ? moveSquareInDirection(toSquare, stm ? NORTH : SOUTH) : toSquare;
        addPiece(board, stm ^ 1, capturedPiece, captureSquare);
    } else if (moveType == CASTLE) {
        bool isKingSideCastle = toSquare > fromSquare;
        Square rookFromSquare = isKingSideCastle ? moveSquareInDirection(toSquare  , EAST) : moveSquareInDirection(toSquare  , WEST + WEST);
        Square rookToSquare   = isKingSideCastle ? moveSquareInDirection(fromSquare, EAST) : moveSquareInDirection(fromSquare, WEST       );
        movePiece(board, stm, ROOK, rookToSquare, rookFromSquare);
    } 
}

bool isLegalMove(const ChessBoard *restrict board, Move move) {
    Square fromSquare = getFromSquare(move);
    Square toSquare = getToSquare(move);
    MoveType moveType = getMoveType(move);
    Colour stm = board->sideToMove;

    if (moveType == CASTLE) {
        Direction towardsKing = toSquare > fromSquare ? WEST : EAST;
        Bitboard occupied = getOccupiedSquares(board);
        while (toSquare != fromSquare) {
            if (attackersTo(board, toSquare, stm, occupied)) return false;
            toSquare = moveSquareInDirection(toSquare, towardsKing);
        }
        return true;
    }

    Square kingSquare = getKingSquare(board, stm);
    Bitboard fromSquareBB = squareToBitboard(fromSquare);
    if (moveType == EN_PASSANT) {
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
    
    return !(board->history->pinnedPieces & fromSquareBB) || fullLine[fromSquare][toSquare] & squareToBitboard(kingSquare);
}

bool isPseudoMove(const ChessBoard *restrict board, Move move) {
    MoveObject moveList[256];
    MoveObject *startList = moveList;
    MoveObject *endList = createMoveList(board, moveList, CAPTURES);
    endList = createMoveList(board, endList, NON_CAPTURES);
    while (startList < endList) {
        if (startList->move == move) return true;
        startList++;
    }
    return false;
}
