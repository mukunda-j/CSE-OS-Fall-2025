#ifndef DISPATCH_H
#define DISPATCH_H

#include "sim.h"

/* Which scheduler to use */
typedef enum {
    DISP_FIFO = 0,
    DISP_SJF,          /* non-preemptive */
    DISP_SRTCF,        /* preemptive SRTF */
    DISP_RR,           /* preemptive Round Robin */
    DISP_PR            /* Priority Queue */
} DispatchAlgo;

/* Function pointer type for any scheduler */
typedef void (*DispatchFn)(CPU* cpu, Queue* ready);

/* Get a scheduler by enum */
DispatchFn dispatch_get(DispatchAlgo algo);

/* Optional: name helper */
const char* dispatch_name(DispatchAlgo algo);

/* Concrete policies */
void dispatch_fifo(CPU* cpu, Queue* ready);
void dispatch_sjf(CPU* cpu, Queue* ready);\
void dispatch_srtcf(CPU* cpu, Queue* ready);
void dispatch_rr(CPU* cpu, Queue* ready, int quantum);
void dispatch_priority(CPU* cpu, Queue* ready);

#endif
