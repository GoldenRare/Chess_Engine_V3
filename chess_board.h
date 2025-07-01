#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include <stdint.h>
#include "utility.h"

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

extern const char *SQUARE_NAME[];
extern const char PROMOTION_NAME[];

static inline Key getPositionKey(const ChessBoard *restrict board) {
    return board->history->positionKey;
}

static inline Bitboard getPieces(const ChessBoard *restrict board, Colour c, PieceType pt) {
    return board->pieces[c][pt];
}

static inline Bitboard getOccupiedSquares(const ChessBoard *restrict board) {
    return board->pieces[WHITE][ALL_PIECES] | board->pieces[BLACK][ALL_PIECES];
}

static inline Bitboard getCheckers(const ChessBoard *restrict board) {
    return board->history->checkers;
}

static inline Square getKingSquare(const ChessBoard *restrict board, Colour c) {
    return bitboardToSquare(getPieces(board, c, KING));
}

// TODO: Technically should be when half move is 100 and not checkmate
// TODO: Threefold repetition, greater or equal to 8
// TODO: Sufficient material
// TODO: Stalemate
// TODO: Null pointer checks needed if the previous positions are not there such as setting FEN to not start
static inline bool isDraw(const ChessBoard *restrict board) {
    /*if (board->history->halfmoveClock >= 8) {
        Key current = getPositionKey(board);
        ChessBoardHistory *previous = board->history->previous->previous->previous->previous;
        int repetitions = current == previous->positionKey ? 2 : 1;
        int count = previous->halfmoveClock;
        while (count >= 2) {
            previous = previous->previous->previous;
            if (current == previous->positionKey && ++repetitions == 3) return true;
            count -= 2;
        }
    }*/
    return board->history->halfmoveClock > 100;
}

// Clears the board, but DOES NOT recursively clear the ChessBoardHistory, only clears the first one.
static inline void clearBoard(ChessBoard *restrict board) {
    *board->history = (ChessBoardHistory) {0};
    *board          = (ChessBoard       ) {0};
}

static inline bool isPathClear(Square from, Square to, Bitboard occupied) {
    return !(inBetweenLine[from][to] & occupied);
}

void initializeChessBoard();

// Caller is responsible for ensuring the board and history struct are zeroed and
// that the history pointer lasts for the necessary duration.
void parseFEN(ChessBoard *board, ChessBoardHistory *history, const char *restrict fen);
void getFEN(const ChessBoard *restrict board, char *restrict destination);

void makeMove(ChessBoard *board, ChessBoardHistory *history, Move move);
void undoMove(ChessBoard *restrict board, Move move);
bool isLegalMove(const ChessBoard *restrict board, Move move);

// TODO: Need to find minimum validation to assert correctness
bool isPseudoMove(const ChessBoard *restrict board, Move move);

#endif
