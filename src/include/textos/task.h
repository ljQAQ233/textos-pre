#ifndef __TASK_H__
#define __TASK_H__

// #include <Cpu.h>

#include <intr.h>

typedef struct {
    u64  r15;
    u64  r14;
    u64  r13;
    u64  r12;
    u64  rbx;
    u64  rbp;
    u64  rip;
} task_frame_t;

#include <textos/klib/list.h>

typedef struct _Task {
    int pid;
    int stat;
    u64 tick;  // default ticks it has
    u64 curr;  // current ticks it has
    
    u64 sleep; // sleeping process has been spent currently

    struct {
        void *main;
        void *rbp;
    } init;

    task_frame_t *frame;
    intr_frame_t *iframe;

    list_t sleeping;
} task_t;

#define TASK_DIE  0 // Dead
#define TASK_PRE  1 // Prepared
#define TASK_RUN  2 // Running
#define TASK_SLP  3 // Sleep
#define TASK_BLK  4 // Blocked

void task_schedule ();

void task_yield ();

task_t *task_current ();

#define TC_KERN (0 << 0) // ring 0 (kernel)
#define TC_USER (1 << 0) // ring 3 (init / app)
#define TC_TSK1 (1 << 1) // init proc
#define TC_NINT (1 << 2) // mask interrupt :(

task_t *task_create (void *main, int args);

void task_sleep (u64 ticks);

void task_block ();

void task_unblock (int pid);

#endif
