/*
 * sp_connect.c
 *
 *  Created on: 2013-11-19
 *      Author: lfx
 */

#include <stdio.h>
#include "sp_define.h"
#include <memory.h>
#include "sctrl.h"

#include "sp_connect.h"
#include <utl_list.h>
#include <mlog.h>
#include "JvServer.h"

static ListHandle_t sList = NULL;

/**
 *@brief 增加一个连接
 */
int sp_connect_add(SPConnection_t *connection)
{
	if (!sList){
		UtlListParam_t param;
		param.bNeedMutex = 1;
		sList = utl_list_create(&param);
	}
	SPConnection_t *c = malloc(sizeof(SPConnection_t));
	memcpy(c, connection, sizeof(SPConnection_t));
	mlog_write("Connected. Type: [%s] User: [%s] IP: [%s]", c->conType == SP_CON_RTSP ? "rtsp" : "jv", c->user, c->addr);
	utl_list_lock(sList);
	utl_list_add(sList, c);
	utl_list_unlock(sList);

	return 0;
}


/**
 *@关键值为protocol和key，其它值没关系
 */
int sp_connect_del(SPConnection_t *connection)
{
	if (!sList)
		return -1;
	utl_list_lock(sList);

	int ret = -1;
	utl_list_seek_set(sList);
	while(1)
	{
		SPConnection_t *p;
		p = (SPConnection_t *)utl_list_get_next(sList);
		if (p)
		{
//			printf("type: %d, key: 0x%08x\n", p->conType, p->key);
			if (p->conType == connection->conType
					&& p->key == connection->key)
			{
				mlog_write("Disconnected. Type: [%s] User: [%s] IP: [%s]", p->conType == SP_CON_RTSP ? "rtsp" : "jv", p->user, p->addr);
				utl_list_del(sList, p);
				ret = 0;
				break;
			}
		}
		else
			break;
	}

	if (ret != 0)
		printf("Failed find connect: key: %x, addr: %s\n", connection->key, connection->addr);
	utl_list_unlock(sList);
	return ret;
}

/**
 *@关键值为conType和key，其它值没关系
 */
int sp_connect_breakoff(SPConnection_t *connection)
{
	if (!sList)
		return -1;
	utl_list_lock(sList);

	int ret = -1;
	utl_list_seek_set(sList);
	while(1)
	{
		SPConnection_t *p;
		p = (SPConnection_t *)utl_list_get_next(sList);
		if (p)
		{
//			printf("type: %d, key: 0x%08x\n", p->conType, p->key);
			if (p->conType == connection->conType
					&& p->key == connection->key)
			{
				utl_list_unlock(sList);
#ifdef YST_SVR_SUPPORT
				if(gp.bNeedYST)
				{
					JVN_SendDataTo((int)p->key, JVN_CMD_DISCONN, 0, 0, 0, 0);
				}
#endif
				ret = 0;
				break;
			}
		}
		else
			break;
	}

	if (ret != 0)
	{
		printf("Failed find connect: key: %x, addr: %s\n", connection->key, connection->addr);
		utl_list_unlock(sList);
	}
	return ret;
}

/**
 *@brief 获取连接数
 */
int sp_connect_get_cnt(SPConType_e conType)
{
	if (!sList)
		return 0;
	if (conType == SP_CON_ALL)
		return utl_list_get_cnt(sList);

	utl_list_lock(sList);

	utl_list_seek_set(sList);
	int cnt = 0;
	while(1)
	{
		SPConnection_t * p;
		p = (SPConnection_t *)utl_list_get_next(sList);
		if (!p)
			break;
		if (p)
		{
			if (p->conType == conType)
			{
				cnt++;
			}
		}
	}

	utl_list_unlock(sList);
	return cnt;

}

/**
 *@brief 将读指针，指向list的开头
 */
int sp_connect_seek_set()
{
	if (!sList)
		return -1;
	utl_list_seek_set(sList);
	return 0;
}

SPConnection_t *sp_connect_get_next(SPConType_e conType)
{
	SPConnection_t * p;
	if (!sList)
		return NULL;
	if (conType == SP_CON_ALL)
	{
		return utl_list_get_next(sList);
	}

	utl_list_lock(sList);
	while(1)
	{
		p = utl_list_get_next(sList);
		if (!p)
			break;
		if (p->conType == conType)
		{
			break;
		}
	}

	utl_list_unlock(sList);
	return p;
}

void sp_test(void)
{
	SPConnection_t con;
	con.conType = SP_CON_JOVISION;
	con.key = 1;
	strcpy(con.addr, "192.168.11.22");
	strcpy(con.protocol, "jovision");
	strcpy(con.user, "admin");
	sp_connect_add(&con);
	printf("connected now: %d\n", sp_connect_get_cnt(SP_CON_ALL));

	con.key = 2;
	sp_connect_add(&con);
	printf("connected now: %d\n", sp_connect_get_cnt(SP_CON_ALL));
	con.key = 3;
	sp_connect_add(&con);
	printf("connected now: %d\n", sp_connect_get_cnt(SP_CON_ALL));
	con.conType = SP_CON_RTSP;
	con.key = 2;
	sp_connect_add(&con);

	printf("connected now: %d\n", sp_connect_get_cnt(SP_CON_ALL));
	printf("SP_CON_JOVISION now: %d\n", sp_connect_get_cnt(SP_CON_JOVISION));
	printf("SP_CON_RTSP now: %d\n", sp_connect_get_cnt(SP_CON_RTSP));
	int cnt = sp_connect_get_cnt(SP_CON_ALL);
	//int i;

	sp_connect_seek_set();
	while(1)
	{
		SPConnection_t *p;
		p = sp_connect_get_next(SP_CON_JOVISION);
		if (!p)
			break;
		printf("now delete p:%d, key: %d, \n", p->conType, p->key);
		sp_connect_del(p);
	}
	sp_connect_seek_set();
	while(1)
	{
		SPConnection_t *p;
		p = sp_connect_get_next(SP_CON_RTSP);
		if (!p)
			break;
		printf("now delete p:%d, key: %d, \n", p->conType, p->key);
		sp_connect_del(p);
	}
}

/**
 * @brief 设置rtmp
 * param chn 通道名称
 * param url rtmp结构体，包括是否启动以及服务器地址
 */
int sp_connect_set_rtmp(int chn,SPConnectionRtmp_t *attr)
{
	return 0;
}

/**
 * @brief 判断是否支持全网通协议
 * param void
 */
int sp_connect_extsdk_support(void)
{
	return 0;
}

#include <semaphore.h>

static sem_t sp_sSem;

int sp_connect_init_semaphore()
{
	sem_init(&sp_sSem, 0, 0);

	return 0;
}

int sp_connect_send_semaphore()
{
	sem_post(&sp_sSem);

	return 0;
}

int sp_connect_recv_semaphore(int timeout)
{
	int ret = 0;
	struct timespec ts;
	ts.tv_sec=timeout/1000;   // 重点
	ts.tv_nsec=timeout*1000000%1000000000;
	
	ret = sem_timedwait(&sp_sSem, &ts);
	if(-1 == ret)
	{
		return 0;
	}

	return 1;
}

int sp_connect_uninit_semaphore()
{
	sem_destroy(&sp_sSem);

	return 0;
}


