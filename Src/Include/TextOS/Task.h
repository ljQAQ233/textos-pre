#ifndef __TASK_H__
#define __TASK_H__

// #include <Cpu.h>

typedef struct {
    u64 rbp;
    u64 rip;
} TaskFrame_t;

#include <TextOS/Lib/List.h>

typedef struct _Task {
    int Pid;
    int Stat;
    u64 Tick;
    u64 Curr;
    u64 Sleep;
    TaskFrame_t *Frame;

    List_t Sleeper;
} Task_t;

#define TASK_DIE  0 // Dead
#define TASK_PRE  1 // Prepared
#define TASK_RUN  2 // Running
#define TASK_SLP  3 // Sleep

#endif
