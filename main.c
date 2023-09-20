#include "move_generator.h"
#include "uci.h"

int main () {
    initializeMoveGenerator();
    uciLoop();
    return 0;
}