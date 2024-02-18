#ifndef UCI_H
#define UCI_H

#define MAX_DEPTH 7

#include "chess_board.h"

void uciLoop();
void processUCICommand();
void processIsReadyCommand();
void processBenchmarkCommand();
void processPositionCommand(ChessBoard *board);
void processMoves(ChessBoard *board);
void processGoCommand(ChessBoard *board);
void processSetOptionCommand();
void processHashOption();

#endif