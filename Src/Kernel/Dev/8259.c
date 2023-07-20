/*
  This is for disabling 8259 interrupt controller.
*/

#include <IO.h>

#define MPIC_DATA 0x21
#define SPIC_DATA 0xA1

void PicDisable ()
{
    /* Mask all interrupts in 8259 */
    OutB (SPIC_DATA, 0xFF);
    OutB (MPIC_DATA, 0xFF);
}