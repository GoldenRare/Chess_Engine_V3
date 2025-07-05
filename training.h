#ifndef TRAINING_H
#define TRAINING_H

#include <pthread.h>
#include <stdio.h>

typedef struct TrainingThread {
    pthread_t th;
    FILE *file;
    bool stop;
} TrainingThread;

void* startTraining(void* trainingThread);

#endif
