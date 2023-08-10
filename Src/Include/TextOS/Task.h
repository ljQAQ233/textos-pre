#ifndef __TASK_H__
#define __TASK_H__

// #include <Cpu.h>

typedef struct {
    u64 rbp;
    u64 rip;
} TaskFrame_t;

typedef struct _Task {
    int Pid;
    int Stat;
    u64 Tick;
    u64 Curr;
    TaskFrame_t *Frame;
} Task_t;

#define TASK_DIE  0 // Dead
#define TASK_PRE  1 // Prepared
#define TASK_RUN  2 // Running

#endif
