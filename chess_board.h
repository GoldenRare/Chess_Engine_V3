#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include <stdint.h>
#include "utility.h"
#include "nnue.h"

// Uses a linked list to keep track of the history of the game. 
// Maintains information that is lost when a move is made but also
// information that is expensive to compute so instead of recomputing it is saved. 
typedef struct ChessBoardHistory {
    struct ChessBoardHistory *previous; // TODO: Consider making it an array in the struct, or pointer that is sequential
    Key positionKey;
    Bitboard checkers;
    Bitboard pinnedPieces;
    PieceType capturedPiece;
    Square enPassant;
    CastlingRights castlingRights;
    uint8_t halfmoveClock; // TODO: Maybe the type
} ChessBoardHistory;

typedef struct ChessBoard {
    Accumulator accumulator;
    ChessBoardHistory *history;
    Bitboard pieces[COLOURS][PIECE_TYPES];
    PieceType pieceTypes[SQUARES];
    Colour sideToMove;
    uint16_t ply; // TODO: Maybe the type
} ChessBoard;

// Indexing the same square will return 0. Example: fullLine[e4][e4] == 0
extern Bitboard fullLine[SQUARES][SQUARES];

// Includes the endpoints as well
extern Bitboard inBetweenLine[SQUARES][SQUARES];

static inline Key getPositionKey(const ChessBoard *restrict board) {
    return board->history->positionKey;
}

static inline Square getEnPassant(const ChessBoard *restrict board) {
    return board->history->enPassant;
}

static inline Bitboard getCheckers(const ChessBoard *restrict board) {
    return board->history->checkers;
}

static inline Bitboard getPieces(const ChessBoard *restrict board, Colour c, PieceType pt) {
    return board->pieces[c][pt];
}

// TODO: Is it worth having one lookup rather than ORing?
static inline Bitboard getOccupiedSquares(const ChessBoard *restrict board) {
    return board->pieces[WHITE][ALL_PIECES] | board->pieces[BLACK][ALL_PIECES];
}

// TODO: Is it worth maintaining the king square in struct rather than compute?
static inline Square getKingSquare(const ChessBoard *restrict board, Colour c) {
    return bitboardToSquare(getPieces(board, c, KING));
}

// TODO: Include more scenarios if necessary
static inline bool insufficientMaterial(const ChessBoard *restrict board) {
    return populationCount(getOccupiedSquares(board)) == 2;
}

// TODO: Technically should be when half move is 100 and not checkmate
// TODO: Threefold repetition, greater or equal to 8
// TODO: Stalemate
// TODO: Null pointer checks needed if the previous positions are not there such as setting FEN to not start
// TODO: More for search, but repetition by history vs repetition by search tree transpose
static inline bool isDraw(const ChessBoard *restrict board) {
    // TODO: Twofold repetition now risky since we can sometimes prevent looking deeply
    if (board->history->halfmoveClock >= 4) {
        Key current = getPositionKey(board);
        ChessBoardHistory *previous = board->history->previous->previous->previous->previous;
        if (current == previous->positionKey) return true;
        for (int count = previous->halfmoveClock; count >= 2; count -= 2) {
            previous = previous->previous->previous;
            if (current == previous->positionKey) return true;
        }
    }
    return board->history->halfmoveClock > 100 || insufficientMaterial(board);
}

/*static inline bool isPseudoMove(const ChessBoard *restrict board, Move move) {
    Square fromSquare = getFromSquare(move);
    Bitboard stmPieces = getPieces(board, board->sideToMove, board->pieceTypes[fromSquare]);
    return stmPieces & squareToBitboard(fromSquare) && ~stmPieces & squareToBitboard(getToSquare( ));
}*/

void initializeChessBoard();

// Caller is responsible for ensuring the board and history struct are zeroed and
// that the history pointer lasts for the necessary duration.
void parseFEN(ChessBoard *board, ChessBoardHistory *history, const char *restrict fen);
void getFEN(const ChessBoard *restrict board, char *restrict destination);

void makeMove(ChessBoard *board, ChessBoardHistory *newState, Move move);
void undoMove(ChessBoard *restrict board, Move move);
bool isLegalMove(const ChessBoard *restrict board, Move move);

// TODO: Need to find minimum validation to assert correctness
bool isPseudoMove(const ChessBoard *restrict board, Move move);

#endif
