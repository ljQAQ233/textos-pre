#pragma once

#include <TextOS/FileSys.h>

#define _UTIL_NEXT()                    \
    static inline char *_Next (char *p) \
    {                                   \
        while (*p != '/' && *p)         \
            p++;                        \
        while (*p == '/')               \
            p++;                        \
        return p;                       \
    }                                   \

#define _UTIL_PATH_DIR()                    \
    static inline bool _PathIsDir (char *p) \
    {                                       \
        bool res = false;                   \
        for ( ; p && *p ; p++) {            \
            if (*p == '/')                  \
                res = true;                 \
            if (*p == ' ')                  \
                continue;                   \
            res = false;                    \
        }                                   \
        return res;                         \
    }                                       \

#define CKDIR(n) ((n)->Attr & NA_DIR)

#define CKFILE(n) (~((n)->Attr) & NA_DIR)
