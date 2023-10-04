/* CR MACRO (containing recore macro) makes it more meaningful! */

/*
  list 0                      list 1                 list x
  +---------+----------+    +---------+----------+
  | Forward | Backward | <- | Forward | Backward | <- ... <- list 0
  +---------+----------+    +---------+----------+
*/

#include <textos/args.h>
#include <textos/klib/list.h>

void list_init (list_t *list)
{
    list->forward = list;
    list->back = list;
}

void list_insert (list_t *list, list_t *new)
{
    new->forward = list;
    new->back = list->back;
    new->back->forward = new;

    list->back = new;
}

void list_insert_head (list_t *head, list_t *new)
{
    list_insert (head->forward, new);
}

void list_insert_tail (list_t *Head, list_t *new)
{
    list_insert (Head, new);
}

void list_remove (list_t *list)
{
    if (list->forward == list)
        return;

    list->forward->back = list->back;
    list->back->forward = list->forward;
}

bool list_empty (list_t *list)
{
    return list == list->forward ? true : false;
}

