#ifndef __TASK_H__
#define __TASK_H__

// #include <Cpu.h>

typedef struct {
    u64  r15;
    u64  r14;
    u64  r13;
    u64  r12;
    u64  rbx;
    u64  rbp;
    u64  rip;
} TaskFrame_t;

typedef struct _Task {
    int Pid;
    int Stat;
    u64 Tick;
    u64 Curr;

    struct {
        void *Main;
    } Init;

    TaskFrame_t *Frame;
} Task_t;

#define TASK_DIE  0 // Dead
#define TASK_PRE  1 // Prepared
#define TASK_RUN  2 // Running

void TaskSwitch ();

void TaskYield ();

Task_t *TaskCurr ();

Task_t *TaskCreate (void *Main);

#endif
