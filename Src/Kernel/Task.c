#include <Cpu.h>
#include <TextOS/Task.h>
#include <TextOS/Memory/VMM.h>

#include <TextOS/Debug.h>
#include <TextOS/Assert.h>

#include <string.h>

/* TODO : Make it included by Kernel proc */
struct _LIST _Sleeper;

#define TASK_MAX  16

#define TASK_PAGE (1)
#define TASK_SIZ  (PAGE_SIZ * TASK_PAGE)

/* Each task has this def tick - 1 */
#define TASK_TICKS 1

Task_t *Table[TASK_MAX];

#define TASK_FREE NULL

static int _TaskFree ()
{
    for (int i = 0; i < TASK_MAX ;i++)
        if (Table[i] == TASK_FREE)
            return i;

    return -1;
}

/* Register tasks into table */
static Task_t *_TaskCreate (u32 Args)
{
    int Pid;
    Task_t *Task;

    if ((Pid = _TaskFree()) < 0)
        return NULL;

    u16 PageFlgs = PE_P | PE_RW;
    if (Args & TC_USER)
        PageFlgs |= PE_US;
    Task = VMM_AllocPages (TASK_PAGE, PageFlgs);
    Task->Pid = Pid;

    Table[Pid] = Task;

    return Task;
}

static _ofp void _TaskStart ()
{
    __asm__ volatile (
        "sti\n"
        "movq  %0, %%rax\n"
        "pushq %%rax\n"
        "ret": : "m"(TaskCurr()->Init.Main)
        );
}

Task_t *TaskCreate (void *Main, u32 Args)
{
    Task_t *Task = _TaskCreate (Args);

    if (Args & TC_INIT) {

        void *Stack = (void *)Task + TASK_SIZ;
        TaskFrame_t *Frame = Stack - sizeof(TaskFrame_t);
        
        Task->Init.Main = Main;
        Task->Init.Rbp  = Frame;

        Frame->rbp = (u64)Stack;
        Frame->rip = (u64)_TaskStart;

        Task->Frame = Frame;
        Task->Stat  = TASK_PRE;
    } else {
        Task->Stat = TASK_RUN;
    }

    Task->Tick = TASK_TICKS;
    Task->Curr = TASK_TICKS;

    memset (Task->Fm.File, 0, sizeof(Task->Fm.File));

    return Task;
}

static void _TaskKernel ()
{
    Task_t *Krn = TaskCreate (NULL, TC_KERN);
    ASSERTK (Krn->Pid == PID_KERNEL);
}

static int _Curr;

static inline Task_t *_TaskNext ()
{
    Task_t *Task = NULL;

    for (int i = _Curr, j = 0; i < TASK_MAX && j < _Curr + TASK_MAX
            ;i = (i + 1) % TASK_MAX, j++)
    {
        if (Table[i] == TASK_FREE)
            continue;
        if (Table[i]->Stat == TASK_PRE) {
            _Curr = i;
            return Table[i];
        }
    }

    return NULL;
}

Task_t *TaskCurr ()
{
    return Table[_Curr];
}

extern void __TaskSwitch (TaskFrame_t *Next, TaskFrame_t **Curr);

void _UpdateSleep ();

void TaskSwitch ()
{
    Task_t *Curr, *Next;

    Curr = TaskCurr(); /* Get curr first becase _TaskNext() will change `_Curr` */
    if (!Curr)
        return;
    
    _UpdateSleep();
    
    if (Curr->Stat != TASK_RUN)   // If it is not running now, then sched
        goto Sched;               // Because a running task may have some time ticks
    if (Curr->Curr-- == 0)        // Update time ticks, if it is zero after this time, set it to origin ticks
        Curr->Curr = Curr->Tick;  // Recovery -> gain ticks again
    else
        return;                   // It hasn't ran out of all ticks yet, make it run again

Sched:
    if (!(Next = _TaskNext()))
        return;

    if (Curr->Stat == TASK_RUN)   // The current task may be a sleeping task
        Curr->Stat = TASK_PRE;    // Only the running task which will be switched out
    Next->Stat = TASK_RUN;

    __TaskSwitch (Next->Frame, &Curr->Frame);
}

void TaskYield ()
{
    TaskSwitch();
}

void _InsertSleep (Task_t *Task)
{
    ListInsert (&_Sleeper, &Task->Sleeper);
}

void _UpdateSleep ()
{
    if (ListEmpty (&_Sleeper)) // 没有任务在睡觉呢...
        return;

    for (List_t *l = _Sleeper.Back; l != &_Sleeper ;l = l->Back) {
        Task_t *Task = CR (l, Task_t, Sleeper);
        if (Task->Pid == _Curr) {
            continue;
        }
        if (--Task->Sleep == 0) {   // 坑死老子啦！2的64次方的大整数是什么 O.o
            Task->Stat = TASK_PRE;
            ListRemove (l);
        }
    }
}

void TaskBlock ()
{
    Task_t *Curr = TaskCurr();

    Curr->Stat = TASK_BLK;

    DEBUGK ("Task %d was blocked!\n", Curr->Pid);
    
    TaskSwitch();
}

void TaskUnblock (int Pid)
{
    Task_t *Task = Table[Pid];
    ASSERTK (Task != NULL);

    Task->Stat = TASK_PRE;
}

void TaskSleep (u64 Ticks)
{
    ASSERTK (Ticks != 0); // 专门防止 24h 工作制

    Task_t *Curr = TaskCurr();
    ASSERTK (Curr != NULL);

    Curr->Stat = TASK_SLP;
    Curr->Sleep = Ticks;

    _InsertSleep (Curr);

    TaskSwitch();
}

#include <TextOS/Console.h>
#include <TextOS/Console/PrintK.h>

#include <Irq.h>
#include <TextOS/Dev.h>

void ProcA ()
{
    ConsoleClear();

    u8 Buffer[512];
    UNINTR_AREA ({
        Dev_t *Disk = DevLookupByType (DEV_BLK, DEV_IDE);
        Disk->BlkRead (Disk, 0, Buffer, 1);

        PrintK ("\n--- DATA DUMP START ---\n");
        for (int i = 0 ; i < sizeof(Buffer) ; i++) {
            PrintK ("%#x, ", Buffer[i]);
        }
        PrintK ("\n---  DATA DUMP END  ---\n");
    });
}

void ProcB ()
{
    while (true) {
        UNINTR_AREA ({
            PrintK ("B");
            // TaskSleep (5);
        });
    }
}

void TaskInit ()
{
    for (int i = 0; i < TASK_MAX ;i++)
    {
        Table[i] = TASK_FREE;
    }

    _TaskKernel();

    _Curr = 0;

    /* _Sleeper 迭代 停止于其本身, 而不对空指针作检查 */
    ListInit (&_Sleeper);

    // TaskCreate (ProcA);
    // TaskCreate (ProcB);
}

