#include "chess_board.h"
#include "move_generator.h"
#include "utility.h"
#include "attacks.h"

static inline bool isPathClear(Square from, Square to, Bitboard occupied) {
    return !(inBetweenLine[from][to] & occupied);
}

static inline MoveObject* generatePromotions(MoveObject *restrict moveList, Square fromSq, Square toSq) {
    setMove(moveList++, fromSq, toSq, QUEEN_PROMOTION );
    setMove(moveList++, fromSq, toSq, ROOK_PROMOTION  );
    setMove(moveList++, fromSq, toSq, BISHOP_PROMOTION);
    setMove(moveList++, fromSq, toSq, KNIGHT_PROMOTION);
    return moveList;
}

static MoveObject* generatePawnMoves(const ChessBoard *restrict board, MoveObject *restrict moveList, Bitboard validSquares, MoveGenerationStage stage) {
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

        if (getEnPassant(board) != NO_SQUARE) {
            Bitboard enPassantCapturers = getPawnAttacks(board->sideToMove ^ 1, getEnPassant(board)) & pawnsNotOn7thRank;
            while (enPassantCapturers) setMove(moveList++, bitboardToSquareWithReset(&enPassantCapturers), getEnPassant(board), EN_PASSANT);
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

static MoveObject* generatePieceMoves(const ChessBoard *restrict board, MoveObject *restrict moveList, Bitboard validSquares, PieceType pt) {
    Bitboard stmPieces = getPieces(board, board->sideToMove, pt);
    Bitboard occupied = getOccupiedSquares(board);
    while (stmPieces) {
        Square fromSq = bitboardToSquareWithReset(&stmPieces);
        Bitboard validAttacks = getAttacks(pt, occupied, fromSq) & validSquares;
        while (validAttacks) setMove(moveList++, fromSq, bitboardToSquareWithReset(&validAttacks), QUIET);
    }
    return moveList;
}

static MoveObject* generateNonKingMoves(const ChessBoard *restrict board, MoveObject *restrict moveList, Bitboard validSquares, MoveGenerationStage stage) {
    moveList = generatePawnMoves (board, moveList, validSquares, stage );
    moveList = generatePieceMoves(board, moveList, validSquares, KNIGHT);
    moveList = generatePieceMoves(board, moveList, validSquares, BISHOP);
    moveList = generatePieceMoves(board, moveList, validSquares, ROOK  );
    moveList = generatePieceMoves(board, moveList, validSquares, QUEEN );
    return moveList;
}

MoveObject* createMoveList(const ChessBoard *restrict board, MoveObject *restrict moveList, MoveGenerationStage stage) {
    Bitboard validSquares = stage == CAPTURES ? getPieces(board, board->sideToMove ^ 1, ALL_PIECES)
                                              : ~getOccupiedSquares(board);
    Bitboard checkers = getCheckers(board);
    if (!checkers) {
        moveList = generateNonKingMoves(board, moveList, validSquares, stage);
        if (stage == NON_CAPTURES) moveList = generateCastleMoves(board, moveList);
    } else if (populationCount(checkers) == 1)
        moveList = generateNonKingMoves(board, moveList, validSquares & inBetweenLine[getKingSquare(board, board->sideToMove)][bitboardToSquare(checkers)], stage);
    return generatePieceMoves(board, moveList, validSquares, KING);
}

MoveObject* generateCastleMoves(const ChessBoard *restrict board, MoveObject *restrict moveList) {
    Colour stm = board->sideToMove;
    CastlingRights stmRights = stm ? BLACK_RIGHTS : WHITE_RIGHTS;
    stmRights &= board->history->castlingRights;
    if (stmRights) { 
        const Square knightSquare[CASTLING_SIDES][COLOURS] = {{G1, G8}, {B1, B8}};
        const Square castlePathStartSquare[CASTLING_SIDES][COLOURS] = {{F1, F8}, {D1, D8}};
        const Square toSquare[CASTLING_SIDES][COLOURS] = {{G1, G8}, {C1, C8}};
        const CastlingRights cr[CASTLING_SIDES] = {KINGSIDE, QUEENSIDE};
        for (size_t i = 0; i < CASTLING_SIDES; i++) { // First kingside, then queenside.
            if ((cr[i] & stmRights) && isPathClear(castlePathStartSquare[i][stm], knightSquare[i][stm], getOccupiedSquares(board))) 
                setMove(moveList++, getKingSquare(board, stm), toSquare[i][stm], CASTLE); // +2 == offset to convert to MoveType == KINGSIDE_CASTLE/QUEENSIDE_CASTLE
        }
    }
    return moveList;
}

bool anyLegalMoves(const ChessBoard *restrict board) {
    MoveObject moveList[MAX_MOVES];
    MoveObject *endList = createMoveList(board, moveList, CAPTURES);
    endList = createMoveList(board, endList, NON_CAPTURES);
    for (MoveObject *moveObj = moveList; moveObj != endList; moveObj++) 
        if (isLegalMove(board, moveObj->move)) return true;
    return false;
}
