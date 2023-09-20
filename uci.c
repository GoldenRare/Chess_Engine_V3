#include <stdio.h>
#include <string.h>
#include "uci.h"
#include "benchmark.h"

static const char *quit = "quit";
static const char *uci = "uci";
static const char *isready = "isready";
static const char *benchmark = "benchmark";

void uciLoop() {
    char input[256]; // TODO: Figure out max size
    char *token = "";

    while (!token || strcmp(token, quit)) {
        fgets(input, sizeof(input), stdin);
        size_t length = strlen(input);
        if (length && input[length - 1] == '\n') input[length - 1] = '\0';

        token = strtok(input, " ");
        if (token == NULL) continue;

        if (strcmp(token, uci) == 0) processUCICommand();
        else if (strcmp(token, isready) == 0) processIsReadyCommand();
        else if (strcmp(token, benchmark) == 0) processBenchmarkCommand();

        char *tokenHelper = token;
        while (tokenHelper != NULL) tokenHelper = strtok(NULL, " ");
    }
    
}

void processUCICommand() {
    printf("id name GoldenRareBOT V3\n");
    printf("id author Deshawn Mohan-Smith\n");
    printf("uciok\n");
}

void processIsReadyCommand() {
    printf("readyok\n");
}

void processBenchmarkCommand() {
    runBenchmark();
}