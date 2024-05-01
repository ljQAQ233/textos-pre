#pragma once

#include <TextOS/FileSys.h>

extern int __VrtFs_Open (Node_t *This, Node_t **Node, const char *Path, u64 Args);

extern int __VrtFs_Read (Node_t *This, void *Buffer, size_t Siz, size_t Offset);

extern int __VrtFs_Write (Node_t *This, void *Buffer, size_t Siz, size_t Offset);

extern int __VrtFs_Close (Node_t *This);

extern int __VrtFs_ReadDir (Node_t *Node);

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

