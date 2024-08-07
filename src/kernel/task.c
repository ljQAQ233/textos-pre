#include <cpu.h>
#include <textos/task.h>
#include <textos/mm/vmm.h>
#include <textos/assert.h>

#include <string.h>

#define TASK_MAX  16

#define TASK_PAGE (1)
#define TASK_SIZ  (PAGE_SIZ * TASK_PAGE)

/* Each task has this def tick -> 1 */
#define TASK_TICKS 1

task_t *table[TASK_MAX];

#define TASK_FREE NULL

static int _getfree ()
{
    for (int i = 0; i < TASK_MAX ;i++)
        if (table[i] == TASK_FREE)
            return i;

    return -1;
}

/* Register tasks into table */
static task_t *_task_create (int args)
{
    int pid;
    task_t *tsk;

    if ((pid = _getfree()) < 0)
        return NULL;

    u16 map_flgs = PE_P | PE_RW;
    if (args & TC_USER)
        map_flgs |= PE_US;
    tsk = vmm_allocpages (TASK_PAGE, map_flgs);
    tsk->pid = pid;

    table[pid] = tsk;

    return tsk;
}

#include <gdt.h>
#include <intr.h>

intr_frame_t *build_iframe (task_t *tsk, int args)
{
    void *stack = (void *)tsk + TASK_SIZ;
    intr_frame_t *iframe = stack - sizeof(intr_frame_t);

    // very good code, makes my test passed
    iframe->rax = 0xaaaa;
    iframe->rbx = 0xbbbb;
    iframe->rcx = 0xcccc;
    iframe->rdx = 0xdddd;
    iframe->rdi = 0x1d1d;
    iframe->rsi = 0x1e1e;
    iframe->r8  = 0x8888;
    iframe->r9  = 0x9999;
    iframe->r10 = 0x1010;
    iframe->r11 = 0x1111;
    iframe->r12 = 0x1212;
    iframe->r13 = 0x1313;
    iframe->r14 = 0x1414;
    iframe->r15 = 0x1515;
    
    iframe->rbp = (u64)stack;
    iframe->rsp = (u64)stack;

    iframe->rflags = (1 << 9);

    if (args & (TC_KERN | TC_TSK1))
        iframe->cs = KERN_CODE_SEG << 3;
    else
        iframe->cs = (USER_CODE_SEG << 3) | 3;

    if (args & TC_NINT)
        iframe->rflags &= ~(1 << 9);

    return tsk->iframe = iframe;
}

task_frame_t *build_tframe (task_t *tsk, int args)
{
    void *stack = (void *)tsk + TASK_SIZ;
    task_frame_t *frame = stack - sizeof(intr_frame_t) - sizeof(task_frame_t);

    frame->rip = (u64)intr_exit;

    return tsk->frame = frame;
}

task_t *task_create (void *main, int args)
{
    task_t *tsk = _task_create(args);

    build_iframe (tsk, args)->rip = (u64)main;
    build_tframe (tsk, args);

    tsk->init.main = main;
    tsk->init.rbp = (void *)tsk->iframe->rbp;

    tsk->stat  = TASK_PRE;
    
    tsk->tick = TASK_TICKS;
    tsk->curr = TASK_TICKS;

    return tsk;
}

static void _task_kern ()
{
    task_create (NULL, TC_KERN)->stat = TASK_RUN;
}

static int _curr;

static inline task_t *_getnext ()
{
    task_t *tsk = NULL;

    for (int i = _curr, j = 0; i < TASK_MAX && j < _curr + TASK_MAX
            ;i = (i + 1) % TASK_MAX, j++)
    {
        if (table[i] == TASK_FREE)
            continue;
        if (table[i]->stat == TASK_PRE) {
            _curr = i;
            return table[i];
        }
    }

    return NULL;
}

task_t *task_current ()
{
    return table[_curr];
}

extern void __task_switch (task_frame_t *next, task_frame_t **curr);

static void update_sleep ();

void task_schedule ()
{
    task_t *curr, *next;
    
    curr = task_current(); /* Get curr first becase _getnext() will change `_curr` */
    if (!curr)
        return;

    update_sleep();
    
    if (curr->stat != TASK_RUN)   // If it is not running now, then sched
        goto Sched;               // Because a running task may have some time ticks
    if (curr->curr-- == 0)        // Update time ticks, if it is zero after this time, set it to origin ticks
        curr->curr = curr->tick;  // Recovery -> gain ticks again
    else
        return;                   // It hasn't ran out of all ticks yet, make it run again

Sched:
    if (!(next = _getnext()))
        return;

    if (curr->stat == TASK_RUN)   // The current task may be a sleeping task
        curr->stat = TASK_PRE;    // Only the running task which will be switched out
    next->stat = TASK_RUN;

    __task_switch (next->frame, &curr->frame);
}

void task_yield ()
{
    task_schedule();
}

struct list _sleeping;

static void sleep_insert (task_t *task)
{
    list_insert (&_sleeping, &task->sleeping);
}

static void update_sleep ()
{
    if (list_empty (&_sleeping)) // 没有任务在睡觉呢...
        return;

    for (list_t *l = _sleeping.back ; l != &_sleeping ; l = l->back)
    {
        task_t *task = CR(l, task_t, sleeping);
        if (task->pid == _curr)
            continue;
        if (--task->sleep == 0) {   // 坑死老子啦！2的64次方的大整数是什么 o.o
            task->stat = TASK_PRE;
            list_remove (l);
        }
    }
}

void task_sleep (u64 ticks)
{
    ASSERTK (ticks != 0); // 专门防止 24h 工作制

    task_t *curr = task_current();
    ASSERTK (curr != NULL);

    curr->stat = TASK_SLP;
    curr->sleep = ticks;

    sleep_insert(curr);
    task_schedule();
}

void task_block ()
{
    task_t *curr = task_current();
    curr->stat = TASK_BLK;
    task_schedule();
}

void task_unblock (int pid)
{
    task_t *tsk = table[pid];
    ASSERTK (tsk != NULL);
    tsk->stat = TASK_PRE;
}

#include <textos/dev.h>
#include <textos/printk.h>

#define N 5000

static int a = 0;

extern void ide_read (u32 lba, void *data, u8 cnt);

void proc_a ()
{
    u8 buf[512];
    dev_t *ide = dev_lookup_type (DEV_BLK, DEV_IDE);
    ide->bread (ide, 0, buf, 1);
    for (int i = 0 ; i < 512 ; i++)
        printk ("%02x", buf[i]);
    printk ("\n");

    while (true) { }
}

void proc_b ()
{
    for (int i = 0 ; i < N ; i++)
        printk ("PROC[#%d] a = %d\n", task_current()->pid, a++);
    while (true) { }
}

void task_init ()
{
    for (int i = 0; i < TASK_MAX ;i++)
    {
        table[i] = TASK_FREE;
    }

    list_init (&_sleeping);

    _task_kern();

    _curr = 0;

    // task_create (proc_a);
    // task_create (proc_b);
}

