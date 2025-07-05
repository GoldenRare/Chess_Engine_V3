#ifndef TRAINING_H
#define TRAINING_H

#include <stdio.h>

typedef struct TrainingThread {
    FILE *file;
    bool stop;
} TrainingThread;

void* startTraining(void* trainingThread);

#endif
