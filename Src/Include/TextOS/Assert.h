#ifndef __ASSERT_H__
#define __ASSERT_H__

void AssertK (
        const char *File,
        const u64  Line,
        const bool State,
        const char *Expr
        );

#define ASSERTK(Expr) \
            AssertK (__FILE__, __LINE__, Expr,#Expr)

#endif