#include <TextOS/TextOS.h>

#include <TextOS/Console.h>
#include <TextOS/Dev/Serial.h>
#include <TextOS/Debug.h>

extern void InitializeAcpi ();
extern void InitializeApic ();
extern void InitializeGdt ();
extern void InitializeIdt ();
extern void MemoryInit ();

extern void ClockInit ();

extern void TaskInit ();

extern void KeyboardInit ();

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

    ClockInit();

    KeyboardInit();

    TaskInit();

    IntrStateEnable();

    while (true);
}
