#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "utl_filecfg.h"
#include "utl_mutex.h"


#ifndef Printf
extern int debugFlag;
#define Printf(fmt...)  \
	do{\
		if(debugFlag){	\
			printf("[%s]:%d ", __FILE__, __LINE__);\
			printf(fmt);} \
	} while(0)

#endif
typedef struct{
	char fname[256];//对应的配置文件的名称
	int realCnt;//实际的配置项数量
	keyvalue_t *list;//配置项列表
	char *fbuffer;//用于存储文件内容，需释放
	int flen;	//fbuffer的长度
	int fend;	//实际的结束位置。后续可能有空余
}fcfg_info_t;

static int fcfg_max = 20;
static fcfg_info_t fcfg_list[20];

static MutexHandle sMutex = NULL;
static void __fcfg_lock()
{
	if (!sMutex)
	{
		sMutex = utl_mutex_create(NULL);
	}
	utl_mutex_lock(sMutex);
}

static void __fcfg_unlock()
{
	utl_mutex_unlock(sMutex);
}

#if 1
#define FUNC_ENTER() __fcfg_lock()
#define FUNC_LEAVE() __fcfg_unlock()
#else
#define FUNC_ENTER() do{__fcfg_lock(); printf("=== %s start, name: %s, tid: %d\n", __func__, fname, utl_thread_getpid());}while(0)
#define FUNC_LEAVE() do{__fcfg_unlock(); printf("=== %s end,   name: %s, tid: %d\n", __func__, fname, utl_thread_getpid());}while(0)
#endif


static fcfg_info_t *__utl_build_fcfg_info(const char *fname, fcfg_info_t *info)
{
	FILE *fp;
	int len;
	int maxCnt = 0;
	int cfgCnt = 0;

	memset(info, 0, sizeof(fcfg_info_t));
	if (!fname)
	{
		Printf("ERROR: __utl_build_fcfg_info fname is NULL\n");
		return NULL;
	}

	fp = fopen(fname, "r");
	if (fp == NULL)
	{
//		Printf("ERROR: __utl_build_fcfg_info Failed open %s, with errno: %s\n", fname, strerror(errno));
		return NULL;
	}
	{
		fseek(fp, 0, SEEK_END);
		len = ftell(fp) + 2;
		fseek(fp, 0, SEEK_SET);
	}
		

	info->fbuffer = malloc(len);
	if (!info->fbuffer)
	{
		Printf("Failed malloc with size: %d\n", len);
		fclose(fp);
		return NULL;
	}

	info->flen = len;
	len = fread(info->fbuffer, 1, len, fp);
	info->fbuffer[len] = '\0';
	info->fend = len+1;

	char *p;

	//计算行数
	p = info->fbuffer;
	while(p && *p)
	{
		//忽略空行和上一行未处理完的回车
		while(*p == '\r' || *p == '\n')
			p++;

		//忽略注释行
		if (*p == '#')
		{
			while(*p && *p != '\r' && *p != '\n')
				p++;
			continue;
		}

		while(*p && *p != '\r' && *p != '\n')
			p++;
		maxCnt++;
	}
//	Printf("maxCnt: %d\n", maxCnt );
	info->list = malloc((maxCnt+1) * sizeof(keyvalue_t));//改正,lck20120730
	if (!info->list)
	{
		Printf("Failed malloc with size: %d\n", maxCnt * sizeof(fcfg_info_t));
		fclose(fp);
		free(info->fbuffer);
		info->fbuffer = NULL;
		return NULL;
	}
	
	p = info->fbuffer;
	while(p && *p)
	{
		//忽略空行和上一行未处理完的回车
		while(*p == '\r' || *p == '\n')
			p++;

		//忽略注释行
		if (*p == '#')
		{
			while(*p && *p != '\r' && *p != '\n')
				p++;
			continue;
		}

		if (cfgCnt > maxCnt)
		{
			Printf("Count calculated ERROR: %d , maxCnt: %d\n",cfgCnt , maxCnt);
			break;
		}

		//获取KEY
		info->list[cfgCnt].key = p;
		p = strchr(p, '=');
		if (p != NULL)
		{
			*p = '\0';
			p++;
			info->list[cfgCnt].value = p;
			while(*p && *p != '\r' && *p != '\n')
				p++;
			if (*p)
				*p = '\0';
			p++;
		}
		else
			info->list[cfgCnt].value = NULL;
		cfgCnt++;
	}

	info->realCnt = cfgCnt;
	strncpy(info->fname, fname, sizeof(info->fname));
	fclose(fp);
	return info;
}

static fcfg_info_t *_utl_fcfg_get(const char *fname, int build)
{
	static int inited = 0;
	int i;
	int freeIndex = -1;

	if (!inited)
	{
		memset(&fcfg_list, 0, sizeof(fcfg_info_t) * fcfg_max);
		inited = 1;
	}
	for (i=0;i<fcfg_max;i++)
	{
		if (strcmp(fname, fcfg_list[i].fname) == 0)
		{
			return &fcfg_list[i];
		}
		else if (fcfg_list[i].fname[0] == '\0' && freeIndex == -1)
		{
			freeIndex = i;
		}
	}
	if (freeIndex == -1)
	{
		return NULL;
	}

	if (build)
		return __utl_build_fcfg_info(fname, &fcfg_list[freeIndex]);
	return NULL;
}

static keyvalue_t *_utl_fcfg_get_value(fcfg_info_t *info, const char *key)
{
	int i;

	if (!info)
	{
		//Printf("Failed get cfg file: %s\n", fname);
		return NULL;
	}

	for (i=0;i<info->realCnt;i++)
	{
		if (strcmp(key, info->list[i].key) == 0)
			return &info->list[i];
	}
	return NULL;
}

/**
 *@brief 从指定文件中获取KEY值对应的value
 *@param fname 要读取的文件名
 *@param key 要查找的KEY值
 *@param valuebuf 输出参数
 *@param maxlen #valuebuf 的长度
 *
 *@return 成功时为valuebuf，否则为Null
 *
 */
char *utl_fcfg_get_value_ex(const char *fname, const char *key, char *valuebuf, int maxlen)
{
	FUNC_ENTER();
	keyvalue_t *kv;
	fcfg_info_t *info = _utl_fcfg_get(fname, 1);

	if (!info)
	{
		Printf("Failed get cfg file: %s\n", fname);
		FUNC_LEAVE();
		return NULL;
	}

	kv = _utl_fcfg_get_value(info, key);
	if (kv == NULL)
	{
		FUNC_LEAVE();
		return NULL;
	}
	strncpy(valuebuf, kv->value, maxlen);
	FUNC_LEAVE();
	return valuebuf;
}
/**
 *@brief 从指定文件中获取KEY值对应的value
 *@param fname 要读取的文件名
 *@param key 要查找的KEY值
 *
 *@return 查找到的KEY对应的值
 *
 */
char *utl_fcfg_get_value(const char *fname, const char *key)
{
	FUNC_ENTER();
	keyvalue_t *kv;
	fcfg_info_t *info = _utl_fcfg_get(fname, 1);

	if (!info)
	{
		Printf("Failed get cfg file: %s\n", fname);
		FUNC_LEAVE();
		return NULL;
	}

	kv = _utl_fcfg_get_value(info, key);
	if (kv == NULL)
	{
		FUNC_LEAVE();
		return NULL;
	}
	FUNC_LEAVE();
	return kv->value;
}

/**
 *@brief 从指定文件中获取KEY值对应的VALUE，类型为INT
 */
int utl_fcfg_get_value_int(const char *fname, const char *key, unsigned int defval)
{
	char *p = utl_fcfg_get_value(fname, key);
	if (p)
	{
		return atoi(p);
	}
	return defval;
}

/**
 *@brief 将一个字符串，插入到info中
 */
static char *_utl_add_string(fcfg_info_t *info, const char *str)
{
	char *temp;
	int nl;
	int slen = strlen(str)+1;
	if (slen > info->flen - info->fend)
	{
		nl = info->flen + 1024 + slen;
		temp = malloc(nl);
		if (temp == NULL)
		{
			Printf("Malloc Memory Failed\n");
			return NULL;
		}
		memcpy(temp, info->fbuffer, info->flen);
		free(info->fbuffer);
		info->flen = nl;

		//修正info->list中内存的指向
		{
			int i;
			for (i=0;i<info->realCnt;i++)
			{
				info->list[i].key = info->list[i].key - info->fbuffer + temp;
				info->list[i].value = info->list[i].value - info->fbuffer + temp;
			}
		}
		info->fbuffer = temp;
	}

	temp = &info->fbuffer[info->fend];
	strcpy(temp, str);
	info->fend += slen;
	return temp;
}

/**
 *@brief 设置某个key的对应的值
 *@param fname 要设置的文件名
 *@param key 要设置的KEY值，这个key值，允许不存在
 *
 *@return 0 成功
 *
 */
int utl_fcfg_set_value(const char *fname, const char *key, char *value)
{
//	int i;
	FUNC_ENTER();
	keyvalue_t *kv;
	fcfg_info_t *info = _utl_fcfg_get(fname, 1);

	if (!info)
	{
		Printf("Failed get cfg file: %s\n", fname);
		FILE *fp = fopen(fname, "wb");
		if (fp == NULL)
		{
			Printf("ERROR: Failed Build file: %s\n", fname);
			FUNC_LEAVE();
			return -1;
		}
		fprintf(fp, "%s=%s\n", key, value);
		fclose(fp);
		fcfg_info_t *info = _utl_fcfg_get(fname, 1);
		if (!info)
		{
			Printf("Failed get cfg file: %s\n", fname);
			FUNC_LEAVE();
			return -1;
		}
		FUNC_LEAVE();
		return 0;
	}

	kv = _utl_fcfg_get_value(info, key);
	if (kv != NULL)
	{
		if (strlen(kv->value) >= strlen(value))
		{
			strcpy(kv->value, value);
		}
		else
		{
			kv->value = _utl_add_string(info, value);
		}
	}
	else
	{
		info->list = realloc(info->list, (info->realCnt+1)* sizeof(keyvalue_t));
		info->list[info->realCnt].key = _utl_add_string(info, key);
		info->list[info->realCnt].value = _utl_add_string(info, value);
		info->realCnt++;
	}
	FUNC_LEAVE();
	return 0;
}

/**
 *@brief 准备getnext
 */
int utl_fcfg_start_getnext(const char *fname)
{
	FUNC_ENTER();
	return 0;
}

/**
 *@brief 结束getnext
 */
int utl_fcfg_end_getnext(const char *fname)
{
	FUNC_LEAVE();
	return 0;
}

/**
 *@brief 从文件中获取下一个配置项
 * 用于轮询文件中所有配置项
 *
 *@param fname 要读取的文件名
 *@return 返回下一个配置项
 *
 */
keyvalue_t *utl_fcfg_get_next(const char *fname, int *current)
{
	fcfg_info_t *info = _utl_fcfg_get(fname, 1);
	keyvalue_t *kv;

	if (!info)
	{
//		Printf("Failed get cfg file: %s\n", fname);
		return NULL;
	}
	if (*current >= info->realCnt || *current < 0)
	{
		return NULL;
	}

	kv = &info->list[*current];
	(*current)++;
	return kv;
}

/**
 *@brief 刷新信息到文件中
 *
 *@param fname 要刷新的文件名
 *@return 0 成功
 *
 */
int utl_fcfg_flush(const char *fname)
{
	FUNC_ENTER();
	FILE *fp;
	int i;
	fcfg_info_t *info;
	info = _utl_fcfg_get(fname, 0);
	if (info == NULL)
	{
		FUNC_LEAVE();
		return 0;
	}

	fp = fopen(fname, "wb");
	if (fp == NULL)
	{
		Printf("ERROR: Failed open file: %s\n", fname);
		FUNC_LEAVE();
		return -1;
	}
	for (i=0;i<info->realCnt;i++)
	{
		fprintf(fp, "%s=%s\n", info->list[i].key, info->list[i].value);
	}

	fclose(fp);
	FUNC_LEAVE();

	return 0;
}

/**
 *@brief 清空文件缓存，当再也不再使用该文件时调用
 *
 *@param fname 要读取的文件名
 *
 *@note 当配置文件有改变时，可以调用此方法，直到刷新该文件的作用
 *
 *@return 0 成功
 */
int utl_fcfg_close(const char *fname)
{
	FUNC_ENTER();
	fcfg_info_t *info = _utl_fcfg_get(fname, 0);
	if (info == NULL)
	{
		Printf("ERROR: Failed find file: %s\n", fname);
		FUNC_LEAVE();
		return -1;
	}

	free(info->fbuffer);
	free(info->list);
	memset(info, 0, sizeof(fcfg_info_t));
	FUNC_LEAVE();
	return 0;
}

//#define MINE_TEST
#ifdef MINE_TEST

int main(int argc, char *argv)
{
	int i;
	keyvalue_t *kv;
	char *value;
	char *key = "Account Add: [%s] Success";
	char *fname = "chinese.lan";

	value = utl_fcfg_get_value(fname, key);
	if (value)
		Printf("utl_fcfg_get_value(%s) == %s\n", key, value);
	else
		Printf("utl_fcfg_get_value(%s) Failed\n", key);

	while(1)
	{
		kv = utl_fcfg_get_next(fname);
		if (kv == NULL)
			break;
		Printf("%s=%s, at position: %d\n", kv->key, kv->value, utl_fcfg_tell(fname));
	}
	utl_fcfg_set_value(fname, "aaa", "bbb");
	utl_fcfg_set_value(fname, "aaa", "bbbcc");
	utl_fcfg_set_value(fname, "aaa", "bbbd");
	utl_fcfg_set_value(fname, "cccc", "eeee");
	utl_fcfg_flush(fname);
	utl_fcfg_close(fname);
	return 0;
}

#endif

