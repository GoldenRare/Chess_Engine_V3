#include <stdint.h>
#include "zobrist.h"
#include "utility.h"

Zobrist zobristHashes;

void initializeZobrist() {
    uint64_t seed = 1070372;
    for (PieceType pt = PAWN; pt < ALL_PIECES; pt++) {
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