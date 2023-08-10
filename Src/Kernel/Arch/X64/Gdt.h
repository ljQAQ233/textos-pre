#ifndef __GDT_H__
#define __GDT_H__

/* Export some settings about gdt */
#define __SEG_KERN_CODE 0x08
#define __SEG_KERN_DATA 0x10
#define __SEG_USER_CODE 0x18
#define __SEG_USER_DATA 0x20

#define __SEG_KERN_STACK 0x10

#endif
