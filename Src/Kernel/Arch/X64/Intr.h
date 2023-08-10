#ifndef __INTR_H__
#define __INTR_H__

#include <Cpu.h>

typedef void (*IntrCaller_t)(u8 Vector, u64 ErrorCode, IntrFrame_t *Intr, RegFrame_t *Reg);

#define __INTR_FUNC(Name) void Name(u8 Vector, u64 ErrorCode, IntrFrame_t *Intr, RegFrame_t *Reg)

void IntrRegister (u8 Vector, IntrCaller_t Func);

#endif
