#ifndef UCI_H
#define UCI_H

#include <stddef.h>
#include <stdint.h>

typedef struct UCI_Configuration {
    size_t hashSize;
    uint8_t threads;
} UCI_Configuration;

void uciLoop();

#endif
