#define R_DATA 0x60
#define R_STAT 0x64
#define R_CMD  0x64

// #define S_OUT (1)
// #define S_IN  (1 << 1)
// #define S_SYS (1 << 2)

#define CMD_PORT1_OFF 0xAD
#define CMD_PORT1_ON  0xAE
#define CMD_PORT2_OFF 0xA7
#define CMD_PORT2_ON  0xA8

#define CMD_PORT1_TEST 0xAB
#define CMD_PORT2_TEST 0xA9

#define CMD_W_CTL  0x60 // Controller Configuration Byte
#define CMD_R_CTL  0x20

#define CMD_W_CTLOUT 0xD1 // PS/2 Controller Output Port
#define CMD_R_CTLOUT 0xD0

/* Controller Configuration Byte */
#define CTL_INT1_ON (1)
#define CTL_INT2_ON (1 << 1)
#define CTL_SYS_PS  (1 << 2)
#define CTL_CLOCK1  (1 << 4)
#define CTL_CLOCK2  (1 << 5)
#define CTL_TRSAN1  (1 << 6) // First PS/2 port transiation

#define S_IN_FULL  (1)
#define S_OUT_FULL (1 << 1)
#define S_SYS      (1 << 2)
#define S_CTL_CMD  (1 << 3)
#define S_TIMEOUT  (1 << 6)
#define S_PERROR   (1 << 7)

#include <IO.h>
#include <TextOS/Debug.h>

#include <Irq.h>
#include <Intr.h>

void KeyboardTest (u8 Vector, IntrFrame_t *Intr, RegFrame_t *Reg)
{
    u8 chr = 0;
    while (!(InB(R_STAT) & S_IN_FULL));

    chr = InB(R_DATA);

    DEBUGK ("Pressed : %u\n", chr);
}

__INTR_FUNC (KeyboardHandler)
{
    LApic_SendEOI();

    u8 chr = 0;
    while (!(InB(R_STAT) & S_IN_FULL));
    chr = InB(R_DATA);

    DEBUGK ("Pressed : %u\n", chr);

    IntrStateEnable();
}

#include <TextOS/Panic.h>

void KeyboardInit ()
{
    // 禁用所有设备
    OutB(R_CMD, CMD_PORT1_OFF);
    OutB(R_CMD, CMD_PORT2_OFF);

    // 自检,如果没有通过则不能进入系统.
    OutB(R_CMD, CMD_PORT1_TEST);
    if (!(InB(R_STAT) & S_SYS))
        PANIC ("Keyboard cannot pass self test!\n");
    
    IntrRegister (INT_KEYBOARD, KeyboardHandler);
    IOApicRteSet (IRQ_KEYBOARD, _IOAPIC_RTE(INT_KEYBOARD));

    // 打开端口与中断
    OutB(R_CMD, CMD_PORT1_ON);
    OutB(R_CMD, CMD_W_CTL);
    OutB(R_DATA, CTL_INT1_ON);
}

