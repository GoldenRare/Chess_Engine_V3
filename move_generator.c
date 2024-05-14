#include <stdlib.h>
#include <stdbool.h>
#include "chess_board.h"
#include "move_generator.h"
#include "utility.h"

Magic magicTable[MAGIC_INDICES][SQUARES];
Bitboard pawnAttacks[COLOURS][SQUARES];
Bitboard nonSlidingAttacks[NON_SLIDER_ATTACKERS][SQUARES];
Bitboard slidingAttacks[MAX_SLIDING_ATTACKS];

void initializeMoveGenerator() {
    initializeNonSlidingAttacks();
    initializeSlidingAttacks();
}

void initializeNonSlidingAttacks() {
    for (Square sq = 0; sq < SQUARES; sq++) {
        Bitboard sqBB = squareToBitboard(sq);
        pawnAttacks[WHITE][sq] = shiftBitboard(sqBB & ~FILE_H_BB, NORTH_EAST) | shiftBitboard(sqBB & ~FILE_A_BB, NORTH_WEST);
        pawnAttacks[BLACK][sq] = shiftBitboard(sqBB & ~FILE_H_BB, SOUTH_EAST) | shiftBitboard(sqBB & ~FILE_A_BB, SOUTH_WEST);
        nonSlidingAttacks[KNIGHT_ATTACKER][sq] = generateKnightAttacks(sqBB);
        nonSlidingAttacks[KING_ATTACKER][sq] = generateKingAttacks(sqBB);
    }
}

void initializeSlidingAttacks() {
    const Direction slidingDirections[MAGIC_INDICES][NUMBER_OF_SLIDING_DIRECTIONS] = {{NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST}, {NORTH, SOUTH, EAST, WEST}};
    size_t count = 0;

    for (size_t i = 0; i < MAGIC_INDICES; i++) {
        for (Square sq = 0; sq < SQUARES; sq++) {
            Bitboard edges = ((RANK_1_BB | RANK_8_BB) & ~rankBitboardOfSquare(sq)) | ((FILE_A_BB | FILE_H_BB) & ~fileBitboardOfSquare(sq));
            Bitboard relevantOccupancyMask = generateSlidingAttacks(slidingDirections[i], NUMBER_OF_SLIDING_DIRECTIONS, sq, 0) & ~edges;
                                  
            Bitboard occupiedSubset = 0;
            size_t startIndex = count;
            magicTable[i][sq].mask = relevantOccupancyMask;
            magicTable[i][sq].offset = startIndex;
            do {
                Bitboard attacks = generateSlidingAttacks(slidingDirections[i], NUMBER_OF_SLIDING_DIRECTIONS, sq, occupiedSubset);
                slidingAttacks[startIndex + pext(occupiedSubset, relevantOccupancyMask)] = attacks;
                occupiedSubset = (occupiedSubset - relevantOccupancyMask) & relevantOccupancyMask;
                count++;
            } while (occupiedSubset);
        }
    }
}

Bitboard generateKnightAttacks(Bitboard knightSq) {
    return shiftBitboard(shiftBitboard(knightSq &              ~FILE_H_BB, NORTH), NORTH_EAST) 
         | shiftBitboard(shiftBitboard(knightSq & ~FILE_G_BB & ~FILE_H_BB, EAST),  NORTH_EAST)
         | shiftBitboard(shiftBitboard(knightSq & ~FILE_G_BB & ~FILE_H_BB, EAST),  SOUTH_EAST)
         | shiftBitboard(shiftBitboard(knightSq &              ~FILE_H_BB, SOUTH), SOUTH_EAST)
         | shiftBitboard(shiftBitboard(knightSq &              ~FILE_A_BB, SOUTH), SOUTH_WEST)
         | shiftBitboard(shiftBitboard(knightSq & ~FILE_A_BB & ~FILE_B_BB, WEST),  SOUTH_WEST)
         | shiftBitboard(shiftBitboard(knightSq & ~FILE_A_BB & ~FILE_B_BB, WEST),  NORTH_WEST)
         | shiftBitboard(shiftBitboard(knightSq &              ~FILE_A_BB, NORTH), NORTH_WEST);
}

Bitboard generateSlidingAttacks(const Direction directions[], size_t numDirections, Square sq, Bitboard occupied) {
    Bitboard attacks = 0ULL;
    for (size_t i = 0; i < numDirections; i++) {
        Square fromSq = sq;
        Square toSq = moveSquareInDirection(sq, directions[i]);
        Bitboard toSquareBB = shiftBitboard(squareToBitboard(sq), directions[i]);
        while (toSquareBB && isAdjacentSquare(fromSq, toSq)) {
            attacks |= toSquareBB;
            if (toSquareBB & occupied) break;
            toSquareBB = shiftBitboard(toSquareBB, directions[i]);
            fromSq = toSq;
            toSq = moveSquareInDirection(fromSq, directions[i]);
        }
    }
    return attacks;
}

Bitboard generateKingAttacks(Bitboard kingSq) {
    Bitboard attacks = shiftBitboard(kingSq & ~FILE_H_BB, EAST) | shiftBitboard(kingSq & ~FILE_A_BB, WEST);
    kingSq |= attacks;
    return attacks | shiftBitboard(kingSq, NORTH) | shiftBitboard(kingSq, SOUTH);
}

MoveObject* createMoveList(const ChessBoard *board, MoveObject *moveList, MoveGenerationStage stage) {
    Bitboard validSquares = stage == CAPTURES ? getPieces(board, board->sideToMove ^ 1, ALL_PIECES) : ~getOccupiedSquares(board);
    if (!board->checkers) {
        moveList = generateNonKingMoves(board, moveList, validSquares, stage);
        if (stage == NON_CAPTURES) moveList = generateCastleMoves(board, moveList);
    } else if (populationCount(board->checkers) == 1) {
        moveList = generateNonKingMoves(board, moveList, validSquares & inBetweenLine[getKingSquare(board, board->sideToMove)][bitboardToSquare(board->checkers)], stage);
    }
    return generatePieceMoves(board, moveList, validSquares, KING);
}

MoveObject* generateNonKingMoves(const ChessBoard *board, MoveObject *moveList, Bitboard validSquares, MoveGenerationStage stage) {
    moveList = generatePawnMoves (board, moveList, validSquares, stage );
    moveList = generatePieceMoves(board, moveList, validSquares, KNIGHT);
    moveList = generatePieceMoves(board, moveList, validSquares, BISHOP);
    moveList = generatePieceMoves(board, moveList, validSquares, ROOK  );
    moveList = generatePieceMoves(board, moveList, validSquares, QUEEN );
    return moveList;
}

MoveObject* generatePawnMoves(const ChessBoard *board, MoveObject *moveList, Bitboard validSquares, MoveGenerationStage stage) {
    Bitboard stmPawns = getPieces(board, board->sideToMove, PAWN);
    Bitboard pawnsOn7thRank = stmPawns & (board->sideToMove ? RANK_2_BB : RANK_7_BB);
    Bitboard pawnsNotOn7thRank = stmPawns ^ pawnsOn7thRank;

    if (stage == CAPTURES) {
        Direction eastCaptureDirection = board->sideToMove ? SOUTH_EAST : NORTH_EAST;
        Direction westCaptureDirection = board->sideToMove ? SOUTH_WEST : NORTH_WEST;

        Bitboard eastCapturePromotions = shiftBitboard(pawnsOn7thRank    & ~FILE_H_BB, eastCaptureDirection) & validSquares;
        Bitboard westCapturePromotions = shiftBitboard(pawnsOn7thRank    & ~FILE_A_BB, westCaptureDirection) & validSquares;
        Bitboard eastCaptures          = shiftBitboard(pawnsNotOn7thRank & ~FILE_H_BB, eastCaptureDirection) & validSquares;
        Bitboard westCaptures          = shiftBitboard(pawnsNotOn7thRank & ~FILE_A_BB, westCaptureDirection) & validSquares;

        while (eastCapturePromotions) {
            Square toSq = bitboardToSquareWithReset(&eastCapturePromotions);
            moveList = generatePromotions(moveList, moveSquareInDirection(toSq, -eastCaptureDirection), toSq);
        }

        while (westCapturePromotions) {
            Square toSq = bitboardToSquareWithReset(&westCapturePromotions);
            moveList = generatePromotions(moveList, moveSquareInDirection(toSq, -westCaptureDirection), toSq);
        }

        while (eastCaptures) {
            Square toSq = bitboardToSquareWithReset(&eastCaptures);
            setMove(moveList++, moveSquareInDirection(toSq, -eastCaptureDirection), toSq, QUIET);
        }

        while (westCaptures) {
            Square toSq = bitboardToSquareWithReset(&westCaptures);
            setMove(moveList++, moveSquareInDirection(toSq, -westCaptureDirection), toSq, QUIET);
        }

        if (board->enPassant != NO_SQUARE) {
            Bitboard enPassantCapturers = getPawnAttacks(board->sideToMove ^ 1, board->enPassant) & pawnsNotOn7thRank;
                while (enPassantCapturers) {
                    setMove(moveList++, bitboardToSquareWithReset(&enPassantCapturers), board->enPassant, EN_PASSANT_CAPTURE);
                }
        }
    } else {
        Direction pawnPush = board->sideToMove ? SOUTH : NORTH;
        Bitboard relative4thRank = board->sideToMove ? RANK_5_BB : RANK_4_BB;
        Bitboard promotions           = shiftBitboard(pawnsOn7thRank   , pawnPush)                   & validSquares;
        Bitboard pawnsAbleToPush      = shiftBitboard(pawnsNotOn7thRank, pawnPush)                   & ~getOccupiedSquares(board);
        Bitboard pawnsAbleToPushTwice = shiftBitboard(pawnsAbleToPush  , pawnPush) & relative4thRank & validSquares;
        pawnsAbleToPush &= validSquares;

        while (promotions) {
            Square toSq = bitboardToSquareWithReset(&promotions);
            moveList = generatePromotions(moveList, moveSquareInDirection(toSq, -pawnPush), toSq);
        }

        while (pawnsAbleToPush) {
            Square toSq = bitboardToSquareWithReset(&pawnsAbleToPush);
            setMove(moveList++, moveSquareInDirection(toSq, -pawnPush), toSq, QUIET);
        }

        while (pawnsAbleToPushTwice) {
            Square toSq = bitboardToSquareWithReset(&pawnsAbleToPushTwice);
            setMove(moveList++, moveSquareInDirection(toSq, 2 * -pawnPush), toSq, DOUBLE_PAWN_PUSH);
        }
    }
    return moveList;
}

MoveObject* generatePieceMoves(const ChessBoard *board, MoveObject *moveList, Bitboard validSquares, PieceType pt) {
    Bitboard stmPieces = getPieces(board, board->sideToMove, pt);
    Bitboard occupied = getOccupiedSquares(board);
    while (stmPieces) {
        Square fromSq = bitboardToSquareWithReset(&stmPieces);
        Bitboard validAttacks = getAttacks(pt, occupied, fromSq) & validSquares;
        while (validAttacks) setMove(moveList++, fromSq, bitboardToSquareWithReset(&validAttacks), QUIET);
    }
    return moveList;
}

MoveObject* generateCastleMoves(const ChessBoard *board, MoveObject *moveList) {
    Colour stm = board->sideToMove;
    CastlingRights stmRights = stm ? BLACK_RIGHTS : WHITE_RIGHTS;
    stmRights &= board->castlingRights;
    if (stmRights) { 
        const Square knightSquare[CASTLING_SIDES][COLOURS] = {{G1, G8}, {B1, B8}};
        const Square castlePathStartSquare[CASTLING_SIDES][COLOURS] = {{F1, F8}, {D1, D8}};
        const Square toSquare[CASTLING_SIDES][COLOURS] = {{G1, G8}, {C1, C8}};
        const CastlingRights cr[CASTLING_SIDES] = {KINGSIDE, QUEENSIDE};
        for (size_t i = 0; i < CASTLING_SIDES; i++) { // First kingside, then queenside.
            if ((cr[i] & stmRights) && isPathClear(castlePathStartSquare[i][stm], knightSquare[i][stm], getOccupiedSquares(board))) {
                setMove(moveList++, getKingSquare(board, stm), toSquare[i][stm], i + 2); // +2 == offset to convert to MoveType == KINGSIDE_CASTLE/QUEENSIDE_CASTLE
            }
        }
    }
    return moveList;
}