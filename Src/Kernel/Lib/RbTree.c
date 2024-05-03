#include <TextOS/Lib/RbTree.h>
    
struct _RbNode;
typedef struct _RbNode RbNode_t;

struct _RbNode
{
    bool      Color;
    u64       Key;
    void     *Payload;
    RbNode_t *Left;
    RbNode_t *Right;
};

#define RED   true
#define BLACK false

#include <TextOS/Memory/Malloc.h>

#define NODE_INIT(n, k, c, p, l, r) \
    do {                            \
        (n).Key = k;                \
        (n).Color = c;              \
        (n).Payload = p;            \
        (n).Left = l;               \
        (n).Right = r;              \
    } while (false);                \

RbTree_t *RbTreeInit (RbTree_t *Rb)
{
    if (!Rb)
        Rb = MallocK (sizeof(RbTree_t));

    Rb->Root = NULL;
    Rb->Depth = 0;

    return Rb;
}

//

static inline bool CkRed (RbNode_t *Node)
{
    return Node ? Node->Color : BLACK;
}

#define BOOL_SWAP(a, b)   \
    do {                   \
        bool t = b;        \
        b = a;             \
        a = t;             \
    } while (false);       \

static inline RbNode_t *RotateLeft (RbNode_t *Node)
{
    RbNode_t *Right = Node->Right;
    Node->Right = Right->Left;             // 接过右子节点的左孩子
    Right->Left = Node;                    // 交换位置 : 右子节点上升, 原节点成为孩子
    BOOL_SWAP(Right->Color, Node->Color);  // 颜色交换
    
    return Right;
}

static inline RbNode_t *RotateRight (RbNode_t *Node)
{
    RbNode_t *Left = Node->Left;
    Node->Left = Left->Right;             // 接过左子节点的右孩子
    Left->Right = Node;                   // 交换位置 : 左子节点上升, 原节点成为孩子
    BOOL_SWAP(Left->Color, Node->Color);  // 颜色交换
    
    return Left;
}

static inline void FlipColor (RbNode_t *Node)
{
    if (Node->Left)
        Node->Left->Color = !Node->Left->Color;
    if (Node->Right)
        Node->Right->Color = !Node->Right->Color;
    Node->Color = !Node->Color;
}

/*
   许多的教程使用的是 Key 来充当查找的关键,
   在 payload 没有改变的情况下还可以用 payload + cmp 函数的方式来查找
*/

static RbNode_t *Insert (RbNode_t *Node, u64 Key, void *Payload, size_t Depth)
{
    if (!Node) {
        Node = MallocK (sizeof(RbNode_t));
        NODE_INIT(*Node, Key, RED, Payload, NULL, NULL);

        return Node;
    }

    if (Key < Node->Key) // 往左走
        Node->Left = Insert (Node->Left, Key, Payload, Depth + 1);
    else                 // 往右走
        Node->Right = Insert (Node->Right, Key, Payload, Depth + 1);

    /*
       对三中情况的处理, 这个可以使用状态机模型来描述
    */
    if (CkRed (Node->Right) && !CkRed (Node->Left)) {
        Node = RotateLeft  (Node);
    }
    if (CkRed (Node->Left) && CkRed (Node->Left->Left)) {
        Node = RotateRight (Node);
    }
    if (CkRed (Node->Left) && CkRed (Node->Right)) {
        FlipColor (Node);
    }

    return Node;
}

void RbTreeInsert (RbTree_t *Rb, u64 Key, void *Payload)
{
    Rb->Root = Insert (Rb->Root, Key, Payload, 0);
    ((RbNode_t*)Rb->Root)->Color = BLACK;
}

//

/* the trick to look up data in the rbtree is same as that of BST */
static RbNode_t *Search (RbNode_t *Node, u64 Key)
{
    if (!Node)
        return NULL;
    if (Key < Node->Key)
        return Search (Node->Left, Key);
    else if (Key > Node->Key)
        return Search (Node->Right, Key);

    /* Has been found */
    return Node;
}

void *RbTreeSearch (RbTree_t *Rb, u64 Key)
{
    RbNode_t *Node = Search (Rb->Root, Key);
    return Node ? Node->Payload : NULL;
}

//

bool RbTreeEmpty (RbTree_t *Rb)
{
    return Rb->Root == NULL;
}

#include <TextOS/Debug.h>

static void Dump (RbNode_t *Node, size_t Level)
{
    if (!Node) {
        DEBUGK ("%*q [ ] nul\n", Level, ' ');
        return ;
    }
    
    DEBUGK ("%*q [%c] %p\n", Level, ' ', Node->Color ? 'R' : 'B', Node->Payload);

    Dump (Node->Left, Level + 1);
    Dump (Node->Right, Level + 1);
}

void __RbTreeDump (RbTree_t *Rb)
{
    Dump (Rb->Root, 0);
}

void __Test_RbTree ()
{
    RbTree_t *Tree = RbTreeInit (NULL);

    char A[] = "Hello world";
    RbTreeInsert (Tree, 0, A);
    char B[] = "Bilibili, cheers";
    RbTreeInsert (Tree, 1, B);
    char C[] = "C is the best lang";
    RbTreeInsert (Tree, 2, C);
    char D[] = "Leave all behind";
    RbTreeInsert (Tree, 3, D);
    
    __RbTreeDump (Tree);

    for (int i = 0 ; i < 4 ; i++) {
        char *Str = RbTreeSearch (Tree, i);
        if (Str)
            DEBUGK ("%d : %s\n", i, Str);
    }
}

// TODO: Delete node
