#pragma once

typedef struct {
    void    *Buffer; // Main body
    size_t  Elem;    // Element size
    size_t  Head;    // Head index
    size_t  Tail;    // Tail index
    size_t  Max;     // Max index   
    size_t  Siz;     // The ring buffer siz
} Ring_t;

/* MARK:Use the feature of overflow may gain more improvements */

Ring_t *RingInit (Ring_t *Ring, void *Buffer, size_t Siz, size_t Elem);

void *RingGet (Ring_t *Ring, size_t Idx);

void *RingPop (Ring_t *Ring);

void RingPush (Ring_t *Ring, void *Elem);

bool RingEmpty (Ring_t *Ring);

void RingClear (Ring_t *Ring);
