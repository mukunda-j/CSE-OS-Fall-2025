#include "process.h"
#include "sjf.h"
#include <stdlib.h>

void sjf(struct process **procArray, int* len, int globalTime){
    int shortest = 2147483647;
    int pid;
    for(int i = 0; i <= len; i++){
        if(procArray[i]->remainingTime <= shortest){
            shortest = procArray[i]->remainingTime;
            pid = i;
        }
    }

    procArray[pid]->finishTime = globalTime + procArray[pid]->remainingTime;
    procArray[pid]->remainingTime = 0;

    for(int i = 0; i < len; i++){
        //shifts items toward the front of the "queue"
        procArray[i] = procArray[i+1];
    }
    procArray[*len] = NULL;
}