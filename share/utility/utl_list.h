/*
 * utl_list.h
 *
 *  Created on: 2013-11-19
 *      Author: lfx
 */

#ifndef UTL_LIST_H_
#define UTL_LIST_H_

typedef void * ListHandle_t;

typedef struct{
	int bNeedMutex; //是否需要多线程操作
}UtlListParam_t;

typedef struct _UtlNode_t{
	void *param;
	struct _UtlNode_t *next;
}UtlNode_t;

/**
 *@brief 创建一个LIST
 */
ListHandle_t utl_list_create(UtlListParam_t *param);

/**
 *@brief 增加一个节点
 *
 *@param handle
 *
 *@return
 */
int utl_list_add(ListHandle_t handle, void *param);

/**
 *@brief 删除一个结点
 *
 *@param handle
 *
 *@return
 */
int utl_list_del(ListHandle_t handle, void *param);

/**
 *@brief 获取总数。同时会有#utl_list_seek_set 的效果
 *
 *@param handle
 *
 *@return
 */
int utl_list_get_cnt(ListHandle_t handle);

/**
 *@brief 将LIST指针指向开头
 *
 *@param handle
 *
 *@return
 */
void utl_list_seek_set(ListHandle_t handle);

/**
 *@brief 获取下一个节点
 *
 *@param handle
 *
 *@return
 */
void *utl_list_get_next(ListHandle_t handle);

/**
 *@brief 获取第一个节点
 *
 *@param handle
 *
 *@return 第一个节点的指针
 */
UtlNode_t *utl_list_get_first(ListHandle_t handle);

/**
 *@brief 删除第一个节点
 *
 *@param handle
 *
 *@return 第一个节点的数据
 */
void *utl_list_pop(ListHandle_t handle);

/**
 *@brief
 *
 *@param handle
 *
 *@return
 */
int utl_list_lock(ListHandle_t handle);

/**
 *@brief
 *
 *@param handle
 *
 *@return
 */
int utl_list_unlock(ListHandle_t handle);

#endif /* UTL_LIST_H_ */
