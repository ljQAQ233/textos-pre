#ifndef __CPU_H__
#define __CPU_H__

void ReadGdt (void *Gdtr);

void LoadGdt (void *Gdtr);

void ReloadSegs (u64 Ss, u64 Cs);

#endif

