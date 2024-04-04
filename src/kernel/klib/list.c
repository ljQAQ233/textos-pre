/* CR MACRO (containing recore macro) makes it more meaningful! */

/*
  List 0                      List 1                 List x
  +---------+----------+    +---------+----------+
  | Forward | Backward | <- | Forward | Backward | <- ... <- List 0
  +---------+----------+    +---------+----------+
*/

#include <textos/args.h>
#include <textos/klib/list.h>

void list_init (list_t *list)
{
    list->forward = list;
    list->back = list;
}

void ListInsert (list_t *list, list_t *new)
{
    new->forward = list;
    new->back = list->back;
    new->back->forward = new;

    list->back = new;
}

void ListInsertHead (list_t *Head, list_t *New)
{
    ListInsert (Head->forward, New);
}

void ListInsertTail (list_t *Head, list_t *New)
{
    ListInsert (Head, New);
}

void ListRemove (list_t *List)
{
    if (List->forward == List)
        return;

    List->forward->back = List->back;
    List->back->forward = List->forward;
}

bool ListEmpty (list_t *List)
{
    return List == List->forward ? true : false;
}

