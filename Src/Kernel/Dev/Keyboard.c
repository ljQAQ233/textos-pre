#include <TextOS/Dev.h>
#include <TextOS/Memory/Malloc.h>

#define INPUT_MAX 0x100

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
#define CTL_TRSAN1  (1 << 6) // First PS/2 port transiation (to scancode 1)

/* Status (R_STAT) */
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

#include <TextOS/Lib/Ring.h>

static Ring_t Ring;
static char   Input[INPUT_MAX];

static bool Shift = false;
static bool Alt   = false;
static bool Ctrl  = false;
static bool Caps  = false;

static void _RingInit ()
{
    RingInit (&Ring, Input, sizeof(Input), sizeof(char));
}

static char _Code[][2] = {
    { '\0'    , '\0'},  // (nul)
    { '^'     , '^' },  // ESC
    { '1'     , '!' },
    { '2'     , '@' },
    { '3'     , '#' },
    { '4'     , '$' },
    { '5'     , '%' },
    { '6'     , '^' },
    { '7'     , '&' },
    { '8'     , '*' },
    { '9'     , '(' },
    { '0'     , ')' },
    { '-'     , '_' },
    { '='     , '+' },
    { '\0'    , '\0'}, // BACKSPACE
    { '\t'    , '\t'},
    { 'q'     , 'Q' },
    { 'w'     , 'W' },
    { 'e'     , 'E' },
    { 'r'     , 'R' },
    { 't'     , 'T' },
    { 'y'     , 'Y' },
    { 'u'     , 'U' },
    { 'i'     , 'I' },
    { 'o'     , 'O' },
    { 'p'     , 'P' },
    { '['     , '{' },
    { ']'     , '}' },
    { '\n'    , '\n'},
    { '\0'    , '\0'},
    { 'a'     , 'A' },
    { 's'     , 'S' },
    { 'd'     , 'D' },
    { 'f'     , 'F' },
    { 'g'     , 'G' },
    { 'h'     , 'H' },
    { 'j'     , 'J' },
    { 'k'     , 'K' },
    { 'l'     , 'L' },
    { ';'     , ':' },
    { '\''    , '"' },
    { '`'     , '~' },
    { '\0'    , '\0'}, // LEFTSHIFT
    { '\\'    , '|' },
    { 'z'     , 'Z' },
    { 'x'     , 'X' },
    { 'c'     , 'C' },
    { 'v'     , 'V' },
    { 'b'     , 'B' },
    { 'n'     , 'N' },
    { 'm'     , 'M' },
    { ','     , '<' },
    { '.'     , '>' },
    { '/'     , '?' },
    { '\0'    , '\0'}, // RIGHTSHIFT
    { '*'     , '*' }, // TODO : Check it
    { '\0'    , '\0'}, // LEFTALT
    { ' '     , ' ' }, // SPACE
    { '\0'    , '\0'}, // CAPSLOCK
    { '\0'    , '\0'}, // F1
    { '\0'    , '\0'}, // F2
    { '\0'    , '\0'}, // F3
    { '\0'    , '\0'}, // F4
    { '\0'    , '\0'}, // F5
    { '\0'    , '\0'}, // F6
    { '\0'    , '\0'}, // F7
    { '\0'    , '\0'}, // F8
    { '\0'    , '\0'}, // F9
    { '\0'    , '\0'}, // F10
    { '\0'    , '\0'}, // NUMLOCK
    { '\0'    , '\0'}, // SCRLOCK
    { '7'     , '7' }, // KBD7
    { '8'     , '8' }, // KBD8
    { '9'     , '9' }, // KBD9
    { '-'     , '-' }, // KBD-
    { '4'     , '4' }, // KBD4
    { '5'     , '5' }, // KBD5
    { '6'     , '6' }, // KBD6
    { '+'     , '+' }, // KBD+
    { '1'     , '1' }, // KBD1
    { '2'     , '2' }, // KBD2
    { '3'     , '3' }, // KBD3
    { '0'     , '0' }, // KBD0
    { '.'     , '.' }, // KBD.
    { '\0'    , '\0'}, // (nul)
    { '\0'    , '\0'}, // (nul)
    { '\0'    , '\0'}, // (nul)
    { '\0'    , '\0'}, // F11
    { '\0'    , '\0'}, // F12
};

#include <TextOS/Task.h>

typedef struct {
    Task_t *Task;
    List_t List;
} Wait_t;

static List_t Waiting;

static int Pid = -1;

static void _WaitInit ()
{
    ListInit (&Waiting);
}

__INTR_FUNC (KeyboardHandler)
{
    LApic_SendEOI();

    while (!(InB(R_STAT) & S_IN_FULL));
    u8 Code = InB(R_DATA);

    bool Break = Code & 0x80;
    Code &= Code &~ 0x80;

    /*
       Shift , Ctrl 按键被按下后只会产生两次中断,即
        
        - Press
        - Release
    */
    switch (Code) {
        case 0x2a: // left
        case 0x36: // right
            Shift = !Shift;
            return;
        case 0x1d: // left
            Ctrl = !Ctrl;
            return;
        case 0x3a:
            if (Break) Caps = !Caps;
            return;
        case 0x38: // left
            Alt = Break;
            return;
        // case 0xe0:
        //     if (*((char *)RingGet(&Ring, Ring.Head)) == 0xb8) {
        //         return;
        //     }
    }

    if (Break)
        return;

    /* 如果开了 Shift , 则选取上端字符,
       如果此时 Caps 也开了 且 扫描码表示的是一个字母, 则将 Line 取逻辑非 */
    bool Line = Shift ? 1 : 0;
    if (Caps && 'a' <= _Code[Code][0] && _Code[Code][0] <= 'z')
        Line = !Line;
    char Chr = _Code[Code][Line];

    RingPush (&Ring, &Chr);

    // DEBUGK ("Common key - %d%d%d - %c!\n", Shift, Ctrl, Caps, Chr);

    if (Pid < 0)
        return;

    TaskUnblock (Pid);
}

static char KeyboardGetc ()
{
    Pid = TaskCurr()->Pid;
    while (RingEmpty (&Ring))
        TaskBlock();

    return *((char *)RingPop (&Ring));
}

void KeyboardRead (Dev_t *Dev, char *Buffer, size_t Count)
{
    for (int i = 0 ; i < Count ; i++) {
        char Chr = KeyboardGetc();
        Buffer[i] = Chr;
    }
}

void KeyboardLight (bool Light)
{
}

#include <TextOS/Panic.h>

/*
    The period of configuration mainly includes 2 parts:
      - Cmd
      - Cfg byte

    1. Cmd -> Write cmds to port `R_CMD`
    2. Cfg byte -> Write CMD_W_CTL to `R_CMD` and configuration byte to `R_DATA`
*/

void KeyboardInit ()
{
    // 禁用所有设备
    OutB(R_CMD, CMD_PORT1_OFF);
    OutB(R_CMD, CMD_PORT2_OFF);

    // 自检,如果没有通过则不能进入系统.
    OutB(R_CMD, CMD_PORT1_TEST);
    while (!(InB(R_STAT) & S_IN_FULL));
    if (!(InB(R_STAT) & S_SYS) || !(InB(R_DATA) == 0x00))
        PANIC ("Keyboard cannot pass self test!\n");
    
    // 初始化 环形缓冲区
    _RingInit();

    IntrRegister (INT_KEYBOARD, KeyboardHandler);
    IOApicRteSet (IRQ_KEYBOARD, _IOAPIC_RTE(INT_KEYBOARD));

    // 打开端口与 中断 & 扫描码翻译
    u8 ConfigByte = CTL_INT1_ON | CTL_TRSAN1;
    OutB(R_CMD, CMD_PORT1_ON);
    OutB(R_CMD, CMD_W_CTL);    // Hw, write cfg byte :)
    OutB(R_DATA, ConfigByte);  // Okay, our final cmd

    Dev_t *Kbd = DevNew();
    
    Kbd->Name = "PS/2 Keyboard";
    Kbd->Read  = (void *)KeyboardRead;
    Kbd->Write = NULL;
    Kbd->Type    = DEV_CHAR;
    Kbd->SubType = DEV_KBD;

    DevRegister (Kbd);
}

