/* A list library based on Macro CR */

#pragma once

struct list;
typedef struct list list_t;

struct list {
    list_t *forward;
    list_t *back;
};

void list_init (list_t *list);

void list_insert (list_t *list, list_t *new);

void list_insert_head (list_t *head, list_t *new);

void list_insert_tail (list_t *Head, list_t *new);

void list_remove (list_t *list);

bool list_empty (list_t *list);

#define LIST_INIT(list) ((list_t){   \
      .forward  = (list_t *)&list,   \
      .back     = (list_t *)&list,   \
    })

