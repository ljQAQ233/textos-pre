#ifndef _STDARG_H
#define _STDARG_H

#include <sys/cdefs.h>

__BEGIN_DECLS

typedef __builtin_va_list va_list;

#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap)          __builtin_va_end(ap)
#define va_arg(ap, type)    __builtin_va_arg(ap, type)

__END_DECLS

#endif
