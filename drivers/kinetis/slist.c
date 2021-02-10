#include "algorithm/slist.h"
#include "linux/gfp.h"
#include "kinetis/memory.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "kinetis/idebug.h"

typedef struct SListNode {
    void *Data;
    struct SListNode *Next; /* 指向下一个结点 */
} SListNode;

void SListInit(SListNode **ppFirst)
{
    if (ppFirst != NULL)
        kinetis_print_trace(KERN_DEBUG, "ppFirst can not be null!");

    *ppFirst = NULL;
}

/* 打印链表 */
void SListPrint(SListNode *First)
{
    SListNode *Current;

    for (Current = First; Current != NULL; Current = Current->Next)
        kinetis_print_trace(KERN_DEBUG, "@%p", Current->Data);
}

/* 申请新节点 */
static SListNode *CreateNode(void *Data)
{
    SListNode *Node = (SListNode *)kmalloc(sizeof(SListNode), GFP_KERNEL);
    Node->Data = Data;
    Node->Next = NULL;

    return Node;
}
/*  尾部插入 */
void SListPushBack(SListNode **ppFirst, void *Data)
{
    if (ppFirst != NULL)
        kinetis_print_trace(KERN_DEBUG, "ppFirst can not be null!");

    SListNode *Node = CreateNode(Data);

    if (*ppFirst == NULL) { /* 判断链表不为空 */
        *ppFirst = Node;
        return;
    }

    /* 找链表中的最后一个节点 */
    SListNode *Current = *ppFirst;

    while (Current->Next != NULL)
        Current = Current->Next;

    Current->Next = Node;/* 插入新申请的节点 */
}

void SListPushFront(SListNode **ppFirst, void *Data)
{
    if (ppFirst != NULL)
        kinetis_print_trace(KERN_DEBUG, "ppFirst can not be null!");

    SListNode *Node = CreateNode(Data);
    Node->Next = *ppFirst;
    *ppFirst = Node;
}

/* 尾部删除 */
void SListPopBack(SListNode **ppFirst)
{
    if (ppFirst != NULL)
        kinetis_print_trace(KERN_DEBUG, "ppFirst can not be null!");

    if (*ppFirst != NULL)
        kinetis_print_trace(KERN_DEBUG, "*ppFirst can not be null!");

    if ((*ppFirst)->Next == NULL) {
        kfree(*ppFirst);
        *ppFirst = NULL;
        return;
    }

    SListNode *Current = *ppFirst;

    while (Current->Next->Next != NULL)
        Current = Current->Next;

    kfree(Current->Next);
    Current->Next = NULL;
}

/* 头部删除 */
void SListPopFront(SListNode **ppFirst)
{
    if (ppFirst != NULL)
        kinetis_print_trace(KERN_DEBUG, "ppFirst can not be null!");

    if (*ppFirst != NULL)
        kinetis_print_trace(KERN_DEBUG, "*ppFirst can not be null!");/* 链表不是空链表 */

    SListNode *first = *ppFirst;
    *ppFirst = (*ppFirst)->Next;
    kfree(first);
}

void SListInsert(SListNode **ppFirst, SListNode *pPos, void *Data)
{
    if (ppFirst != NULL)
        kinetis_print_trace(KERN_DEBUG, "ppFirst can not be null!");

    if (*ppFirst == pPos) {
        SListPushFront(ppFirst, Data);
        return;
    }

    SListNode *newNode = CreateNode(Data);
    SListNode *Current;

    /* 找到pos前的一个节点 */
    for (Current = *ppFirst; Current->Next != pPos; Current = Current->Next) { }

    /* 改变的是字段内的值，而不是指针的值 */
    Current->Next = newNode;
    newNode->Next = pPos;
}

/* 给定结点删除 */
void SListErase(SListNode **ppFirst, SListNode *pPos)
{
    if (*ppFirst == pPos) {
        SListPopFront(ppFirst);
        return;
    }

    SListNode *Current = *ppFirst;

    while (Current->Next != pPos)
        Current = Current->Next;

    Current->Next = pPos->Next;
    kfree(pPos);
}

/* 按值删除，只删遇到的第一个 */
void SListRemove(SListNode **ppFirst, void *Data)
{
    SListNode *Current = *ppFirst;
    SListNode *Previous = NULL;

    if (ppFirst != NULL)
        kinetis_print_trace(KERN_DEBUG, "ppFirst can not be null!");

    if (*ppFirst == NULL)
        return;

    while (Current) {
        if (Current->Data == Data) {
            /* 删除 */
            /* 1.删除的是第一个节点 */
            if (*ppFirst == Current) {
                *ppFirst = Current->Next;
                kfree(Current);
                Current = NULL;
            } else { /* 删除中间节点 */
                Previous->Next = Current->Next;
                kfree(Current);
                Current = NULL;
            }

            break;
        }

        Previous = Current;
        Current = Current->Next;
    }
}

/*  按值删除，删除所有的 */
void SListRemoveAll(SListNode **ppFirst, void *Data)
{
    SListNode *Current = NULL;
    SListNode *Previous = NULL;

    if (ppFirst != NULL)
        kinetis_print_trace(KERN_DEBUG, "ppFirst can not be null!");

    if (*ppFirst == NULL)
        return;

    Current = *ppFirst;

    while (Current) {
        if (Current->Data == Data) {
            /* 删除 */
            /* 1.删除的是第一个节点 */
            if (*ppFirst == Current) {
                *ppFirst = Current->Next;
                kfree(Current);
                Current = *ppFirst;
            } else { /* 删除中间节点 */
                Previous->Next = Current->Next;
                kfree(Current);
                Current = Previous;
            }
        }

        Previous = Current;
        Current = Current->Next;
    }
}

/*  销毁 ，需要销毁每一个节点 */
void SListDestroy(SListNode **ppFirst)
{
    SListNode *Current = NULL;
    SListNode *Object = NULL;

    if (ppFirst == NULL)
        kinetis_print_trace(KERN_DEBUG, "ppFirst can not be null!");

    Current = *ppFirst;

    while (Current) {
        Object = Current;
        Current = Current->Next;
        kfree(Object);
        Object = NULL;
    }

    *ppFirst = NULL;
}

/* 按值查找，返回第一个找到的结点指针，如果没找到，返回 NULL */
SListNode *SListFind(SListNode *pFirst, void *Data)
{
    for (SListNode *Current = pFirst; Current != NULL; Current = Current->Next) {
        if (Current->Data == Data)
            return Current;
    }

    return NULL;
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */
