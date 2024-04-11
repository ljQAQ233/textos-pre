/* A list library based on Macro CR */

#pragma once

struct _LIST;
typedef struct _LIST List_t;

struct _LIST {
    List_t *Forward;
    List_t *Back;
};

void ListInit (List_t *List);

void ListInsert (List_t *List, List_t *New);

void ListInsertHead (List_t *Head, List_t *New);

void ListInsertTail (List_t *Head, List_t *New);

void ListRemove (List_t *List);

bool ListEmpty (List_t *List);

#define LIST_INIT(List) ((List_t ){ \
      .Forward  = (List_t *)&List, \
      .Back     = (List_t *)&List, \
    })
