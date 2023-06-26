#ifndef __IO_H__
#define __IO_H__

void OutB (u16 Port, u8 Data);

void OutW (u16 Port, u16 Data);

void OutDW (u16 Port, u32 Data);

u8 InB (u16 Port);

u16 InW (u16 Port);

u32 InDW (u16 Poet);

#endif