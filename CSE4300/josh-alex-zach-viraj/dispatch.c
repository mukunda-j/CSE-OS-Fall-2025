#include "dispatch.h"

DispatchFn dispatch_get(DispatchAlgo algo) {
    switch (algo) {
        case DISP_FIFO:  return dispatch_fifo;
        case DISP_SJF:   return dispatch_sjf;
        case DISP_SRTCF: return dispatch_srtcf;   /* NEW */
        default:         return dispatch_fifo;
    }
}

const char* dispatch_name(DispatchAlgo algo) {
    switch (algo) {
        case DISP_FIFO:  return "FIFO";
        case DISP_SJF:   return "SJF (non-preemptive)";
        case DISP_SRTCF: return "SRTCF (preemptive SRTF)";
        default:         return "unknown";
    }
}

void dispatch_fifo(CPU* cpu, Queue* ready) {
    while (cpu_any_idle(cpu)) {
        Thread* t = q_pop(ready);
        if (!t) break;
        cpu_bind_first_idle(cpu, t);
    }
}

/* SJF (non-preemptive): pick the thread with the smallest *burst_time*
   from Ready for each idle core. Ties break by first-encountered in the queue. */
void dispatch_sjf(CPU* cpu, Queue* ready) {
    while (cpu_any_idle(cpu)) {
        Thread* t = q_pop_min_burst(ready);   // O(n) scan of Ready
        if (!t) break;
        cpu_bind_first_idle(cpu, t);
    }
}

/* ------- SRTCF (preemptive SRTF) ------- */
/* pick core with the largest remaining > threshold; return -1 if none */
static int core_with_largest_remaining_above(const CPU* cpu, int threshold) {
    int best_core = -1, best_rem = -1;
    for (int i = 0; i < cpu->ncores; ++i) {
        Thread* r = cpu->core[i];
        if (!r) continue;
        if (r->remaining > threshold && r->remaining > best_rem) {
            best_rem = r->remaining;
            best_core = i;
        }
    }
    return best_core;
}

void dispatch_srtcf(CPU* cpu, Queue* ready) {
    /* First, fill any idle cores with the smallest-remaining jobs */
    while (cpu_any_idle(cpu)) {
        Thread* t = q_pop_min_remaining(ready);
        if (!t) break;
        cpu_bind_first_idle(cpu, t);
    }

    /* Preempt if the best ready job is better than something running */
    for (;;) {
        Thread* best = q_pop_min_remaining(ready);
        if (!best) break;

        /* If an idle core appeared (due to earlier I/O or completion), bind it */
        int idle = cpu_first_idle(cpu);
        if (idle >= 0) {
            cpu_bind_core(cpu, idle, best);
            continue;
        }

        /* Otherwise, find a running core whose remaining is worse than 'best' */
        int victim = core_with_largest_remaining_above(cpu, best->remaining);
        if (victim >= 0) {
            /* preempt the victim to Ready, run 'best' now */
            preempt_to_ready(cpu, victim, ready);
            cpu_bind_core(cpu, victim, best);
            /* continue loop: there might be another ready job better than some
               other running job */
            continue;
        }

        /* Nowhere to place 'best' (all running <= best). Put it back and stop. */
        q_push(ready, best);
        break;
    }
}