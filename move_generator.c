#include <stdlib.h>
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
    int rankDistance = abs(squareToRank(toSq) - squareToRank(fromSq));
    int fileDistance = abs(squareToFile(toSq) - squareToFile(fromSq));
    return max(rankDistance, fileDistance) == 1;
}

Move* generateAllMoves(const ChessBoard *board, Move *moveList) {
    moveList = generatePawnMoves(board, moveList);
    moveList = generateKnightMoves(board, moveList);
    moveList = generateSlidingMoves(board, moveList, BISHOP, BISHOP_INDEX);
    moveList = generateSlidingMoves(board, moveList, ROOK, ROOK_INDEX);
    moveList = generateSlidingMoves(board, moveList, QUEEN, ROOK_INDEX); // BISHOP_INDEX is computed for Queen in function
    moveList = generateKingMoves(board, moveList);
    return moveList;
}

Move* generatePawnMoves(const ChessBoard *board, Move *moveList) {
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
    Bitboard enPassantCapturers = enPassant != NO_SQUARE ? getPawnAttacks(enemy, enPassant) & pawnsNotOn7thRank : 0;
    Bitboard pawnsAbleToPush = shiftBitboard(pawnsNotOn7thRank, pawnPush) & empty;
    Bitboard pawnsAbleToPushTwice = shiftBitboard(pawnsAbleToPush, pawnPush) & empty & relative4thRank;
    Bitboard promotions = shiftBitboard(pawnsOn7thRank, pawnPush) & empty;

    /* PAWNS ON 7th RANK HANDLING */
    while (pawnsOn7thRank) {
        Square fromSq = bitboardToSquareWithReset(&pawnsOn7thRank);
        Bitboard captures = getPawnAttacks(stm, fromSq) & enemyPieces;
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
    while (pawnsNotOn7thRank) {
        Square fromSq = bitboardToSquareWithReset(&pawnsNotOn7thRank);
        Bitboard captures = getPawnAttacks(stm, fromSq) & enemyPieces;
        while (captures) setMove(moveList++, fromSq, bitboardToSquareWithReset(&captures), CAPTURE);
    }

    while (enPassantCapturers) setMove(moveList++, bitboardToSquareWithReset(&enPassantCapturers), enPassant, EN_PASSANT_CAPTURE);

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

Move* generateKnightMoves(const ChessBoard *board, Move *moveList) {
    Colour stm = getSideToMove(board);
    Bitboard stmKnights = getPieces(board, stm, KNIGHT);
    Bitboard stmPieces = getPieces(board, stm, ALL_PIECES);
    Bitboard enemyPieces = getPieces(board, stm ^ 1, ALL_PIECES);

    while (stmKnights) {
        Square fromSq = bitboardToSquareWithReset(&stmKnights);
        Bitboard validAttacks = getNonSlidingAttacks(KNIGHT_ATTACKER, fromSq) & ~stmPieces;
        Bitboard captures = validAttacks & enemyPieces;
        Bitboard quiets = validAttacks ^ captures;
        
        while (captures) setMove(moveList++, fromSq, bitboardToSquareWithReset(&captures), CAPTURE);
        while (quiets) setMove(moveList++, fromSq, bitboardToSquareWithReset(&quiets), QUIET);
    }
    return moveList;
}

Move* generateSlidingMoves(const ChessBoard *board, Move *moveList, PieceType pt, MagicIndex slidingIndex) {
    Colour stm = getSideToMove(board);
    Bitboard stmMovingPieces = getPieces(board, stm, pt);
    Bitboard stmPieces = getPieces(board, stm, ALL_PIECES);
    Bitboard enemyPieces = getPieces(board, stm ^ 1, ALL_PIECES);
    Bitboard occupied = stmPieces | enemyPieces; 

    while (stmMovingPieces) {
        Square fromSq = bitboardToSquareWithReset(&stmMovingPieces);
        Bitboard validAttacks = getSlidingAttacks(occupied, fromSq, slidingIndex);
        if (pt == QUEEN) validAttacks |= getSlidingAttacks(occupied, fromSq, BISHOP_INDEX);

        Bitboard captures = validAttacks & enemyPieces;
        Bitboard quiets = (validAttacks & ~stmPieces) ^ captures;
        
        while (captures) setMove(moveList++, fromSq, bitboardToSquareWithReset(&captures), CAPTURE);
        while (quiets) setMove(moveList++, fromSq, bitboardToSquareWithReset(&quiets), QUIET);
    }
    return moveList;
}

Move* generateKingMoves(const ChessBoard *board, Move *moveList) {
    Colour stm = getSideToMove(board);
    Bitboard stmKing = getPieces(board, stm, KING);
    Bitboard stmPieces = getPieces(board, stm, ALL_PIECES);
    Bitboard enemyPieces = getPieces(board, stm ^ 1, ALL_PIECES);

    while (stmKing) {
        Square fromSq = bitboardToSquareWithReset(&stmKing);
        Bitboard validAttacks = getNonSlidingAttacks(KING_ATTACKER, fromSq) & ~stmPieces;
        Bitboard captures = validAttacks & enemyPieces;
        Bitboard quiets = validAttacks ^ captures;
        
        while (captures) setMove(moveList++, fromSq, bitboardToSquareWithReset(&captures), CAPTURE);
        while (quiets) setMove(moveList++, fromSq, bitboardToSquareWithReset(&quiets), QUIET);
    }
    return moveList;
}