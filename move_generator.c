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
        while (toSquareBB && isDirectionMaintained(fromSq, toSq)) {
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

bool isDirectionMaintained(Square fromSq, Square toSq) {
    int rankDistance = abs((int) squareToRank(toSq) - (int) squareToRank(fromSq));
    int fileDistance = abs((int) squareToFile(toSq) - (int) squareToFile(fromSq));
    return max(rankDistance, fileDistance) == 1;
}

Move* createMoveList(const ChessBoard *board, Move *moveList) {
    Colour stm = board->sideToMove;
    Square kingSq = getKingSquare(board, stm);
    Bitboard checkers = attackersTo(board, kingSq, stm, getOccupiedSquares(board));
    if (!checkers) {
        moveList = generateAllMoves(board, moveList, ENTIRE_BOARD & ~getPieces(board, stm, ALL_PIECES));
        return generateCastleMoves(board, moveList);
    } else if (populationCount(checkers) == 1) {
        Bitboard validSquares = (inBetweenLine[kingSq][bitboardToSquare(checkers)] | checkers) & ~getPieces(board, stm, ALL_PIECES);
        return generateAllMoves(board, moveList, validSquares);
    } else {
        return generateNonPawnMoves(board, moveList, ENTIRE_BOARD & ~getPieces(board, stm, ALL_PIECES), KING);
    }
}

Move* generateAllMoves(const ChessBoard *board, Move *moveList, Bitboard validSquares) {
    moveList = generatePawnMoves(board, moveList, validSquares);
    moveList = generateNonPawnMoves(board, moveList, validSquares, KNIGHT);
    moveList = generateNonPawnMoves(board, moveList, validSquares, BISHOP);
    moveList = generateNonPawnMoves(board, moveList, validSquares, ROOK);
    moveList = generateNonPawnMoves(board, moveList, validSquares, QUEEN);
    moveList = generateNonPawnMoves(board, moveList, ENTIRE_BOARD & ~getPieces(board, getSideToMove(board), ALL_PIECES), KING);
    return moveList;
}

Move* generatePawnMoves(const ChessBoard *board, Move *moveList, Bitboard validSquares) {
    Colour stm = getSideToMove(board);
    Colour enemy = stm ^ 1;
    Bitboard stmPawns = getPieces(board, stm, PAWN);
    Bitboard enemyPieces = getPieces(board, enemy, ALL_PIECES);
    Bitboard empty = ~getOccupiedSquares(board);
    Square enPassant = getEnPassantSquare(board);

    Direction pawnPush = stm ? SOUTH : NORTH;
    Bitboard relative4thRank = stm ? RANK_5_BB : RANK_4_BB;
    Bitboard relative7thRank = stm ? RANK_2_BB : RANK_7_BB;
    
    Bitboard pawnsOn7thRank = stmPawns & relative7thRank;
    Bitboard pawnsNotOn7thRank = stmPawns ^ pawnsOn7thRank;
    
    Bitboard pawnsAbleToPush = shiftBitboard(pawnsNotOn7thRank, pawnPush) & empty;
    Bitboard pawnsAbleToPushTwice = shiftBitboard(pawnsAbleToPush, pawnPush) & empty & relative4thRank & validSquares;
    pawnsAbleToPush &= validSquares;
    Bitboard promotions = shiftBitboard(pawnsOn7thRank, pawnPush) & empty & validSquares;

    /* PAWNS ON 7th RANK HANDLING */
    while (pawnsOn7thRank) {
        Square fromSq = bitboardToSquareWithReset(&pawnsOn7thRank);
        Bitboard captures = getPawnAttacks(stm, fromSq) & enemyPieces & validSquares;
        while (captures) {
            Square toSq = bitboardToSquareWithReset(&captures);
            setMove(moveList++, fromSq, toSq, QUEEN_PROMOTION_CAPTURE);
            setMove(moveList++, fromSq, toSq, ROOK_PROMOTION_CAPTURE);
            setMove(moveList++, fromSq, toSq, BISHOP_PROMOTION_CAPTURE);
            setMove(moveList++, fromSq, toSq, KNIGHT_PROMOTION_CAPTURE);
        } 
    }

    while (promotions) {
        Square toSq = bitboardToSquareWithReset(&promotions);
        Square fromSq = moveSquareInDirection(toSq, -pawnPush);
        setMove(moveList++, fromSq, toSq, QUEEN_PROMOTION);
        setMove(moveList++, fromSq, toSq, ROOK_PROMOTION);
        setMove(moveList++, fromSq, toSq, BISHOP_PROMOTION);
        setMove(moveList++, fromSq, toSq, KNIGHT_PROMOTION);
    }
    /*                            */

    /* PAWNS NOT ON 7th RANK HANDLING */
    if (enPassant != NO_SQUARE) {
        bool notInCheck = (validSquares | getPieces(board, stm, ALL_PIECES)) == ENTIRE_BOARD;
        if (notInCheck || !(squareToBitboard(moveSquareInDirection(enPassant, pawnPush)) & validSquares)) {
            Bitboard enPassantCapturers = getPawnAttacks(enemy, enPassant) & pawnsNotOn7thRank;
            while (enPassantCapturers) {
                setMove(moveList++, bitboardToSquareWithReset(&enPassantCapturers), enPassant, EN_PASSANT_CAPTURE);
            }
        }
    }

    while (pawnsNotOn7thRank) {
        Square fromSq = bitboardToSquareWithReset(&pawnsNotOn7thRank);
        Bitboard captures = getPawnAttacks(stm, fromSq) & enemyPieces & validSquares;
        while (captures) setMove(moveList++, fromSq, bitboardToSquareWithReset(&captures), CAPTURE);
    } 

    while (pawnsAbleToPushTwice) {
        Square toSq = bitboardToSquareWithReset(&pawnsAbleToPushTwice);
        Square fromSq = moveSquareInDirection(toSq, 2 * -pawnPush);
        setMove(moveList++, fromSq, toSq, DOUBLE_PAWN_PUSH);
    }

    while (pawnsAbleToPush) {
        Square toSq = bitboardToSquareWithReset(&pawnsAbleToPush);
        Square fromSq = moveSquareInDirection(toSq, -pawnPush);
        setMove(moveList++, fromSq, toSq, QUIET);
    }
    /*                                */

    return moveList;
}

Move* generateNonPawnMoves(const ChessBoard *board, Move *moveList, Bitboard validSquares, PieceType pt) {
    Colour stm = getSideToMove(board);
    Bitboard stmPieces = getPieces(board, stm, pt);
    Bitboard enemyPieces = getPieces(board, stm ^ 1, ALL_PIECES);
    Bitboard occupied = getPieces(board, stm, ALL_PIECES) | enemyPieces;
    while (stmPieces) {
        Square fromSq = bitboardToSquareWithReset(&stmPieces);
        Bitboard validAttacks = getAttacks(pt, occupied, fromSq) & validSquares;
        Bitboard captures = validAttacks & enemyPieces;
        Bitboard quiets = validAttacks ^ captures;
        
        while (captures) setMove(moveList++, fromSq, bitboardToSquareWithReset(&captures), CAPTURE);
        while (quiets) setMove(moveList++, fromSq, bitboardToSquareWithReset(&quiets), QUIET);
    }
    return moveList;
}

Move* generateCastleMoves(const ChessBoard *board, Move *moveList) {
    Colour stm = getSideToMove(board);
    CastlingRights stmRights = stm ? BLACK_RIGHTS : WHITE_RIGHTS;
    stmRights &= getCastlingRights(board);
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