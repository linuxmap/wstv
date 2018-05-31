#ifndef __SP_DEFINE_H__
#define __SP_DEFINE_H__


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif

typedef struct{
	int x;
	int y;
	int w;
	int h;
}SPRect_t;

typedef struct{
	int x;
	int y;
}SPVector;

typedef struct{
	int w;
	int h;
}SPRes_t;

//可以打印文件名和行号,lck20100417
#ifndef Printf
#define Printf(fmt...)  \
do{\
	if(1){	\
		printf("[%s]:%d ", __FILE__, __LINE__);\
		printf(fmt);} \
} while(0)
#endif

#ifndef CPrintf
#define CPrintf(fmt...)  \
do{\
	if(1){	\
		printf("\33[31m");\
		printf(fmt);} \
		printf("\33[0m ");\
} while(0)
#endif


#define __FUNC_DBG__() \
	do{ \
		if (2 > 1) \
			printf("\33[31mNOT_FINISHED %s:%s =>%4d: called\33[0m\n", __FILE__, __func__, __LINE__); \
	} while(0)



#endif /* #ifndef __JV_COMMON_H__ */

