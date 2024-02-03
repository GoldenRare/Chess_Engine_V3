#ifndef UCI_H
#define UCI_H

#define MAX_DEPTH 5

#include "chess_board.h"

void uciLoop();
void processUCICommand();
void processIsReadyCommand();
void processBenchmarkCommand();
void processPositionCommand(ChessBoard *board);
void processMoves(ChessBoard *board);
void processGoCommand(ChessBoard *board);

#endif