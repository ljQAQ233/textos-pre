#include <Cpu.h>
#include <Gdt.h>
#include <TextOS/Task.h>
#include <TextOS/Memory/VMM.h>

#include <string.h>

#define TASK_MAX  16

#define TASK_PAGE (1)
#define TASK_SIZ  (PAGE_SIZ * TASK_PAGE)

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
static Task_t *_TaskCreate ()
{
    int Pid;
    Task_t *Task;

    if ((Pid = _TaskFree()) < 0)
        return NULL;

    Task = VMM_AllocPages (TASK_PAGE, PE_P | PE_RW);
    Task->Pid = Pid;

    Table[Pid] = Task;

    return Task;
}

Task_t *TaskCreate (void *Main)
{
    Task_t *Task = _TaskCreate();

    void *Stack = (void *)Task + TASK_SIZ;
    TaskFrame_t *Frame = Stack - sizeof(TaskFrame_t);

    Frame->rbp = (u64)Stack;
    Frame->rip = (u64)Main;

    Task->Frame = Frame;
    Task->Stat  = TASK_PRE;

    return Task;
}

static void _TaskKernel ()
{
    TaskCreate (NULL)->Stat = TASK_RUN;
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

static inline Task_t *_TaskCurr ()
{
    return Table[_Curr];
}

extern void __TaskSwitch (TaskFrame_t *Next, TaskFrame_t **Curr);

void TaskSwitch ()
{
    __asm__ volatile ("cli");
    
    Task_t *Curr = _TaskCurr(); /* Get curr first becase _TaskNext() will change `_Curr` */
    if (!Curr)
        return;

    Task_t *Next = _TaskNext();
    if (!Next)
        return;

    Curr->Stat = TASK_PRE;
    Next->Stat = TASK_RUN;

    __TaskSwitch (Next->Frame, &Curr->Frame);
}

#include <TextOS/Console/PrintK.h>

void ProcA ()
{
    while (true) {
        PrintK ("A");
    }
}

void ProcB ()
{
    while (true) {
        PrintK ("B");
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

    TaskCreate (ProcA);
    TaskCreate (ProcB);
}

