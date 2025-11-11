#ifndef _THREAD_H_
#define _THREAD_H_

struct thread {
    int thread_ID;      // job ID
    int arrival;        // arrival time
    int turnaround;     // turnaround time
    int response;       // response time
    int burst;          // burst time
    int wait;           // wait time
};

#endif /* _THREAD_H_ */
