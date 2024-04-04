/* CR MACRO (containing recore macro) makes it more meaningful! */

/*
  List 0                      List 1                 List x
  +---------+----------+    +---------+----------+
  | Forward | Backward | <- | Forward | Backward | <- ... <- List 0
  +---------+----------+    +---------+----------+
*/

#include <TextOS/Args.h>
#include <TextOS/Lib/List.h>

void ListInit (List_t *List)
{
    List->Forward = List;
    List->Back = List;
}

void ListInsert (List_t *List, List_t *New)
{
    New->Forward = List;
    New->Back = List->Back;
    New->Back->Forward = New;

    List->Back = New;
}

void ListInsertHead (List_t *Head, List_t *New)
{
    ListInsert (Head->Forward, New);
}

void ListInsertTail (List_t *Head, List_t *New)
{
    ListInsert (Head, New);
}

void ListRemove (List_t *List)
{
    if (List->Forward == List)
        return;

    List->Forward->Back = List->Back;
    List->Back->Forward = List->Forward;
}

bool ListEmpty (List_t *List)
{
    return List == List->Forward ? true : false;
}

