#pragma once

struct _RbTree;
typedef struct _RbTree RbTree_t;

struct _RbTree
{
    void   *Root;
    size_t  Depth;
};

/* To provide a set of interl types is not necessary, I think... */

/* Initialize a rbtre, allocate mem if need */
RbTree_t *RbTreeInit (RbTree_t *Rb);

/* Key : search index */
void RbTreeInsert (RbTree_t *Rb, u64 Key, void *Payload);

void *RbTreeSearch (RbTree_t *Rb, u64 Key);
