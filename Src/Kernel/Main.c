#include <TextOS/TextOS.h>

#include <TextOS/Console.h>
#include <TextOS/Dev/Serial.h>
#include <TextOS/Debug.h>
#include <TextOS/Task.h>

extern void InitializeAcpi ();
extern void InitializeApic ();
extern void InitializeGdt ();
extern void InitializeIdt ();
extern void MemoryInit ();

extern void TaskInit ();

extern void DevInit ();
extern void KeyboardInit ();
extern void IdeInit ();
extern void ClockInit ();

static void __ProcInit ();

#include <Irq.h>

void KernelMain ()
{
    ConsoleInit();
    SerialInit();

    InitializeGdt();
    InitializeIdt();

    InitializeAcpi();
    InitializeApic();

    MemoryInit();

    DevInit();
    KeyboardInit();
    IdeInit();
    ClockInit();

    TaskInit();

    Task_t *Tmp = TaskCreate(__ProcInit, TC_INIT | TC_USER);

    IntrStateEnable();

    while (true);
}

#define INIT_PROG "/init.elf"

#include <Gdt.h>
#include <TextOS/Panic.h>
#include <TextOS/User/Elf.h>

extern void InitFileSys ();

static void __ProcInit ()
{
    UNINTR_AREA_START();
    InitFileSys();
    UNINTR_AREA_END();

    Exec_t Info;
    if (ElfLoad (INIT_PROG, &Info) < 0)
        PANIC("Failed to load init!\n");

    Task_t *This = TaskCurr();

    __asm__ volatile (
            "push %0 \n" // ss
            "push %1 \n" // rsp
            "pushfq  \n" // rflags
            "push %2 \n" // cs
            "push %3 \n" // rip
            : :
            "i"((USER_DATA_SEG << 3) | 3), // ss
            "m"(This->Init.Rbp),           // rsp
            "i"((USER_CODE_SEG << 3) | 3), // cs
            "m"(Info.Entry));              // rip
    __asm__ volatile ("iretq");

    PANIC("Init exiting...\n");
}

