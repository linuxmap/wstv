/*
 * mgb28181.c
 *
 *  Created on: 2013-11-11
 *      Author: lfx
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include <jv_common.h>
#include "mgb28181.h"
#include <utl_filecfg.h>
#include <utl_ifconfig.h>
#include "sctrl.h"

static GBRegInfo_t sGbInfo;


static void __read_info(GBRegInfo_t *info)
{
	gb_get_default_param(info);
	info->bEnable = 0;
	info->chnCnt = 1;
	strcpy(info->devid, "34020000001320000001");
	strcpy(info->devpasswd, "12345678");
	info->expires = 3600;
	info->alarminCnt = 1;
	info->keepalive = 30;
	strcpy(info->alarminID[0], "34020000001340000010");

	eth_t eth;
	utl_ifconfig_eth_get(&eth);

	strcpy(info->localip, eth.addr);
	strcpy(info->serverip, "192.168.11.31");

	keyvalue_t *kv;
	int cur = 0;

	utl_fcfg_start_getnext(GB28181_CFG_FILE);
	while(1)
	{
		kv = utl_fcfg_get_next(GB28181_CFG_FILE, &cur);
		if (kv == NULL)
			break;
		Printf("%s=%s\n", kv->key, kv->value);
		if (strcmp(kv->key, "bEnable") == 0)
			info->bEnable = atoi(kv->value);
		else if (strcmp(kv->key, "devid") == 0)
			strcpy(info->devid, kv->value);
		else if (strcmp(kv->key, "devpasswd") == 0)
			strcpy(info->devpasswd, kv->value);
		else if (strcmp(kv->key, "serverip") == 0)
			strcpy(info->serverip, kv->value);
		else if (strcmp(kv->key, "serverport") == 0)
			info->serverport = atoi(kv->value);
		else if (strcmp(kv->key, "localport") == 0)
			info->localport = atoi(kv->value);
		else if (strcmp(kv->key, "expires") == 0)
			info->expires = atoi(kv->value);
		else if (strcmp(kv->key, "chnCnt") == 0)
			info->chnCnt = atoi(kv->value);
		else if (strcmp(kv->key, "alarminCnt") == 0)
			info->alarminCnt = atoi(kv->value);
		else if (strcmp(kv->key, "keepalive") == 0)
			info->keepalive = atoi(kv->value);
		else if (strcmp(kv->key, "GB_EX_refresh") == 0)
			info->expires_refresh = atoi(kv->value);
		else if (strcmp(kv->key, "GB_KA_outtimes") == 0)
			info->keepalive_outtimes = atoi(kv->value);

		int id;
		int ret;
		if (strncmp(kv->key, "chnID", 5) == 0)
		{
			ret = sscanf(kv->key, "chnID%d", &id);
			if (ret == 1 && id >= 0 && id <= MAX_CHN_CNT)
			{
				strcpy(info->chnID[id], kv->value);
			}
			continue;
		}

		if (strncmp(kv->key, "alarminID", 9) == 0)
		{
			ret = sscanf(kv->key, "alarminID%d", &id);
			if (ret == 1 && id >= 0 && id <= MAX_ALARMIN_CNT)
			{
				strcpy(info->alarminID[id], kv->value);
			}
			continue;
		}
	}
	utl_fcfg_end_getnext(GB28181_CFG_FILE);
}

static void __save_info(GBRegInfo_t *info)
{
	char temp[32];
	sprintf(temp, "%d", info->bEnable);
	utl_fcfg_set_value(GB28181_CFG_FILE, "bEnable", temp);
	utl_fcfg_set_value(GB28181_CFG_FILE, "devid", info->devid);
	utl_fcfg_set_value(GB28181_CFG_FILE, "devpasswd", info->devpasswd);
	utl_fcfg_set_value(GB28181_CFG_FILE, "serverip", info->serverip);
	sprintf(temp, "%d", info->serverport);
	utl_fcfg_set_value(GB28181_CFG_FILE, "serverport", temp);
	sprintf(temp, "%d", info->localport);
	utl_fcfg_set_value(GB28181_CFG_FILE, "localport", temp);
	sprintf(temp, "%d", info->expires);
	utl_fcfg_set_value(GB28181_CFG_FILE, "expires", temp);
	sprintf(temp, "%d", info->chnCnt);
	utl_fcfg_set_value(GB28181_CFG_FILE, "chnCnt", temp);
	sprintf(temp, "%d", info->alarminCnt);
	utl_fcfg_set_value(GB28181_CFG_FILE, "alarminCnt", temp);
	sprintf(temp, "%d", info->keepalive);
	utl_fcfg_set_value(GB28181_CFG_FILE, "keepalive", temp);
	sprintf(temp, "%d", info->expires_refresh);
	printf("================%s\n",temp);
	utl_fcfg_set_value(GB28181_CFG_FILE, "GB_EX_refresh", temp);
	sprintf(temp, "%d", info->keepalive_outtimes);
	printf("================%s\n",temp);
	utl_fcfg_set_value(GB28181_CFG_FILE, "GB_KA_outtimes", temp);


	int i;
	for (i=0;i<info->chnCnt;i++)
	{
		char tkey[32];
		sprintf(tkey, "chnid%d", i);
		utl_fcfg_set_value(GB28181_CFG_FILE, tkey, info->chnID[i]);
	}
	for (i=0;i<info->alarminCnt;i++)
	{
		char tkey[32];
		sprintf(tkey, "alarminID%d", i);
		utl_fcfg_set_value(GB28181_CFG_FILE, tkey, info->alarminID[i]);
	}

	utl_fcfg_flush(GB28181_CFG_FILE);
	utl_fcfg_close(GB28181_CFG_FILE);
}

#include <utl_timer.h>

static BOOL bInited = FALSE;

static BOOL __check_ip_timer(int tid, void *param)
{
	eth_t eth;
	if (!bInited)
		return TRUE;
	utl_ifconfig_eth_get(&eth);
	if (0 != strcmp (eth.addr, sGbInfo.localip))
	{
		strcpy(sGbInfo.localip, eth.addr);
		gb_reset_param(&sGbInfo);
	}
	return TRUE;
}


int mgb28181_init(void)
{
	if(hwinfo.bHomeIPC)
		return 0;
	__read_info(&sGbInfo);

	gb_init(&sGbInfo);
	bInited = TRUE;

	utl_timer_create("mgb28181", 60*1000, __check_ip_timer, NULL);
	return 0;
}

int mgb28181_set_param(GBRegInfo_t *param)
{
	if (memcmp(&sGbInfo, param, sizeof(sGbInfo)) == 0)
	{
		return 0;
	}

	memcpy(&sGbInfo, param, sizeof(sGbInfo));
	__save_info(&sGbInfo);
	gb_reset_param(&sGbInfo);
	return 0;
}

int mgb28181_get_param(GBRegInfo_t *param)
{
	memcpy(param, &sGbInfo, sizeof(sGbInfo));
	return 0;
}

int mgb28181_deinit(void)
{
	if(hwinfo.bHomeIPC)
		return 0;
	bInited = FALSE;
	return gb_deinit();
}
