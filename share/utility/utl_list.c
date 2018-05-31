/*
 * utl_list.c
 *
 *  Created on: 2013-11-19
 *      Author: lfx
 */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <pthread.h>

#include "utl_list.h"

typedef struct{
	int bNeedMutex;
	UtlNode_t *header;
	UtlNode_t *cur;
	pthread_mutex_t mutex;
}UtlList_t;

ListHandle_t utl_list_create(UtlListParam_t *param)
{
	UtlList_t *list = malloc(sizeof(UtlList_t));
	if (!list)
	{
		return NULL;
	}

	memset(list, 0, sizeof(UtlList_t));
	list->bNeedMutex = param->bNeedMutex;
	if (list->bNeedMutex)
	{
		pthread_mutex_init(&list->mutex, NULL);
	}

	return (ListHandle_t)list;
}

int utl_list_add(ListHandle_t handle, void *param)
{
	UtlList_t *list = (UtlList_t *)handle;
	UtlNode_t *node = malloc(sizeof(UtlNode_t));
	if (!node)
		return -1;
	memset(node, 0, sizeof(UtlNode_t));
	node->param = param;

	UtlNode_t *p = list->header;
	if (p == NULL)
	{
		list->header = node;
		return 0;
	}
	while(p->next)
	{
		p = p->next;
	}
	p->next = node;

	return 0;
}

int utl_list_del(ListHandle_t handle, void *param)
{
	UtlList_t *list = (UtlList_t *)handle;

	UtlNode_t *p = list->header;
	UtlNode_t *q = NULL;
	while (p)
	{
		if (p->param == param)
		{
			if (list->cur == p)
				list->cur = p->next;
			//头一个
			if (!q)
			{
				list->header = p->next;
			}
			else
			{
				q->next = p->next;
			}
			break;
		}
		q = p;
		p = p->next;
	}

	if (!p)
	{
		printf("Failed find the node\n");
		return -1;
	}
	free(p);
	return 0;
}

UtlNode_t *utl_list_get_first(ListHandle_t handle)
{
	UtlList_t *list = (UtlList_t *)handle;
	return list->header;
}

void *utl_list_pop(ListHandle_t handle)
{
	UtlList_t *list = (UtlList_t *)handle;
	UtlNode_t *p = list->header;
	void *param = NULL;

	if(p)
	{
		if (list->cur == p)
			list->cur = p->next;
		list->header = p->next;
		param = p->param;
		free(p);
	}

	return param;
}

int utl_list_get_cnt(ListHandle_t handle)
{
	int cnt;
	UtlList_t *list = (UtlList_t *)handle;

	UtlNode_t *p = list->header;
	cnt = 0;
	while (p)
	{
		cnt++;
		p = p->next;
	}
	return cnt;
}
//
//void *utl_list_get(ListHandle_t handle, int index)
//{
//	int i;
//	UtlList_t *list = (UtlList_t *)handle;
//
//	UtlNode_t *p = list->header;
//	i = 0;
//	while (p)
//	{
//		if (i == index)
//			return p->param;
//		i++;
//		p = p->next;
//	}
//	return NULL;
//}

/**
 *@brief 将LIST指针指向开头
 *
 *@param handle
 *
 *@return
 */
void utl_list_seek_set(ListHandle_t handle)
{
	UtlList_t *list = (UtlList_t *)handle;

	list->cur = list->header;
}

/**
 *@brief 获取下一个节点
 *
 *@param handle
 *
 *@return
 */
void *utl_list_get_next(ListHandle_t handle)
{
	UtlList_t *list = (UtlList_t *)handle;
	void *param = NULL;

	if (list->cur)
	{
		param = list->cur->param;
		list->cur = list->cur->next;
	}
	return param;
}

int utl_list_lock(ListHandle_t handle)
{
	UtlList_t *list = (UtlList_t *)handle;
	if (!list->bNeedMutex)
		return 0;
	return pthread_mutex_lock(&list->mutex);
}

int utl_list_unlock(ListHandle_t handle)
{
	UtlList_t *list = (UtlList_t *)handle;
	if (!list->bNeedMutex)
		return 0;
	return pthread_mutex_unlock(&list->mutex);
}

