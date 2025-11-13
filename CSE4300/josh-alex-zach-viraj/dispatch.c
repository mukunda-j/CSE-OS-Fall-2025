#include "dispatch.h"

const char* dispatch_name(DispatchAlgo algo) {
    switch (algo) {
        case DISP_FIFO:  return "FIFO";
        case DISP_SJF:   return "SJF (non-preemptive)";
        case DISP_SRTCF: return "SRTCF (preemptive SRTF)";
        case DISP_RR:    return "RR (preemptive)";
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

/* ----------- RR ------------- */
void dispatch_rr(CPU* cpu, Queue* ready, int quantum) {
    if (quantum < 1) quantum = 1;

    /* 1) Preempt expired-slice threads (they hit qrem==0 at end of last tick) */
    for (int i = 0; i < cpu->ncores; ++i) {
        Thread* t = cpu->core[i];
        if (!t) continue;
        if (t->quanta_rem == 0) {
            preempt_to_ready(cpu, i, ready);  // unbind -> Ready tail; t->qrem unchanged (0)
        }
    }

    /* 2) Fill idle cores from Ready; new bindings get a full slice */
    for (int i = 0; i < cpu->ncores; ++i) {
        if (cpu->core[i]) continue;
        Thread* t = q_pop(ready);
        if (!t) break;
        t->quanta_rem = quantum;                 // give fresh quantum
        cpu_bind_core(cpu, i, t);
    }
}

/* ------------ PRIORITY ---------------*/
/* find running core with the worst priority above threshold */
static int core_with_worst_priority_above(const CPU* cpu, int threshold) {
    int victim_core = -1;
    int worst = -1;   /* track largest priority value greater than threshold */
    for (int i = 0; i < cpu->ncores; ++i) {
        Thread* r = cpu->core[i];
        if (!r) continue;
        if (r->priority > threshold && r->priority > worst) {
            worst = r->priority;
            victim_core = i;
        }
    }
    return victim_core;
}

void dispatch_priority(CPU* cpu, Queue* ready) {
    /* fill idle cores first */
    while (cpu_any_idle(cpu)) {
        Thread* t = q_pop_highest_priority(ready);  /* smallest priority value wins */
        if (!t) break;
        cpu_bind_first_idle(cpu, t);
    }

    /* preempt if a ready thread beats some running thread */
    for (;;) {
        Thread* best = q_pop_highest_priority(ready);
        if (!best) break;

        // preempt if the current theres a higher priority thread
        int victim = core_with_worst_priority_above(cpu, best->priority);
        if (victim >= 0) {
            preempt_to_ready(cpu, victim, ready);
            cpu_bind_core(cpu, victim, best);
            continue;
        }

        /* no preemption: put it back and stop */
        q_push(ready, best);
        break;
    }
}