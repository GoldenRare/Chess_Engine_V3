#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include "utility.h"

// TODO: Revisit what is stored in the structs, primarily the pinned pieces
typedef struct ChessBoard {
    Bitboard pieces[COLOURS][PIECE_TYPES];
    Key positionKey;
    Bitboard checkers;
    Bitboard pinnedPieces;
    PieceType pieceTypes[SQUARES];
    Colour sideToMove;
    Square enPassant;
    CastlingRights castlingRights;
} ChessBoard;

// Keeps track of the information that is lost when a move is made
typedef struct IrreversibleBoardState {
    Key positionKey;
    Bitboard checkers;
    Bitboard pinnedPieces;
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

static inline Bitboard getPieces(const ChessBoard *restrict board, Colour c, PieceType pt) {
    return board->pieces[c][pt];
}

// TODO: Revisit if it is worth saving the value in board struct directly
static inline Bitboard getOccupiedSquares(const ChessBoard *restrict board) {
    return board->pieces[WHITE][ALL_PIECES] | board->pieces[BLACK][ALL_PIECES];
}

// TODO: Revisit if it is worth saving the king square directly
static inline Square getKingSquare(const ChessBoard *restrict board, Colour c) {
    return bitboardToSquare(getPieces(board, c, KING));
}

static inline bool isPathClear(Square from, Square to, Bitboard occupied) {
    return !(inBetweenLine[from][to] & occupied);
}

void initializeChessBoard();

// Caller is responsible for ensuring the board is zeroed.
void parseFEN(ChessBoard *restrict board, const char *restrict fen);
void getFEN(const ChessBoard *restrict board, char *restrict destination);

void makeMove(ChessBoard *restrict board, Move move, IrreversibleBoardState *restrict ibs);
void undoMove(ChessBoard *restrict board, Move move, const IrreversibleBoardState *restrict ibs);
bool isLegalMove(const ChessBoard *restrict board, Move move);

// TODO: Need to find minimum validation to assert correctness
bool isPseudoMove(const ChessBoard *restrict board, Move move);

#endif
