#include <TextOS/TextOS.h>

#include <TextOS/Console.h>
#include <TextOS/Dev/Serial.h>
#include <TextOS/Debug.h>

extern void InitializeAcpi ();
extern void InitializeApic ();
extern void InitializeGdt ();
extern void InitializeIdt ();
extern void MemoryInit ();

extern void TaskInit ();

extern void DevInit ();
extern void KeyboardInit ();
extern void IdeInit ();

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

    IntrStateEnable();

    while (true);
}
