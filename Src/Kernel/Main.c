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

    TaskInit();

    Task_t *Tmp = TaskCreate(__ProcInit);
    
    IntrStateEnable();

    while (true);
}

extern void InitFileSys ();

static void __ProcInit ()
{
    UNINTR_AREA_START();
    InitFileSys();
    UNINTR_AREA_END();

    while (true) TaskYield();
}
