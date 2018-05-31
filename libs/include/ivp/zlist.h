/***********************************************************
*zlist.h - doubly linked list head  file
*
* Copyright(c) 2005~
*
*$Date: $ 
*$Revision: $
*
*-----------------------
*$Log: $
*
*01a, 05-12-13 Zhushuchao created
*
************************************************************/
#ifndef __Z_LIST_H_
#define __Z_LIST_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

typedef struct _Z_NODE
{
    struct _Z_NODE *next;
    struct _Z_NODE *prev;
    void *data;
}Z_NODE;

typedef int (* ZCompare)(void *, void *);

typedef struct
{
    int count;
    Z_NODE *head;
    Z_NODE *tail;
    ZCompare Compare;
}Z_LIST;

int ZListInit(Z_LIST * list);
int ZListCount(Z_LIST * list);
int ZListSetCompare(Z_LIST * list, ZCompare Compare);
int ZListAdd(Z_LIST * list, void * data);
int ZListInsert(Z_LIST *list, void *Data);
int ZListDelete(Z_LIST * list, void * data);
int ZListInsertNode(Z_LIST *list, Z_NODE *node);
int ZListAddNode(Z_LIST * list, Z_NODE *node);
int ZListMoveNode(Z_LIST * list, Z_NODE * node);
int ZListDeleteNode(Z_LIST * list, Z_NODE * node);
int ZListClean(Z_LIST * list);
int ZListAppend(Z_LIST * toList, Z_LIST * addList);
int ZListGetIndex(Z_LIST * list, void * data);
void *ZListPop(Z_LIST *list);
void * ZListGetData(Z_LIST * list, int index);
Z_NODE *ZListPopNode(Z_LIST * list);
Z_NODE *ZListMoveData(Z_LIST * list, void *data);
Z_NODE * ZListGetFirstNode(Z_LIST * list);
Z_NODE *ZListGetNextNode(Z_LIST * list, Z_NODE * fromNode);
Z_NODE *ZListGetPrevNode(Z_LIST * list, Z_NODE * fromNode);
Z_NODE *ZListFindNode(Z_LIST *list, void *data);
Z_LIST *ZListCreate();
int ZListDistory(Z_LIST * list);

#ifdef __cplusplus
}
#endif
#endif

