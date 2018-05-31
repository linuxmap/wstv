/*
 * utl_memory.c
 *
 *  Created on: 2014Äê1ÔÂ20ÈÕ
 *      Author: lfx  20451250@qq.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

void *__real_malloc(size_t);
void *__wrap_malloc(size_t c);
void __wrap_free(void * ptr);
void __real_free(void * ptr);
void *__wrap_realloc(void *ptr,size_t size);
void __real_realloc(void * ptr,size_t size);

#ifdef __cplusplus
}
#endif


#if 1
void* operator new(size_t size)
{
//	printf("new running\n");
        char* p= (char*)malloc(size);
	return p;
}
void operator delete(void *p)
{
        free(p);
}

void* operator new[](size_t size)
{
//	printf("new []\n");
	char* p= (char*)malloc(size);
	return p;
}

void operator delete[](void* p)
{
	free(p);
}

#endif
#define MEMTEST_MAGIC 0x12345678

typedef struct{
	unsigned int magic;
	unsigned char magiclist[128];
	size_t size;
	int threadid;
	void *next;
}MemStart_t;

typedef struct{
	unsigned int magic;
	unsigned char magiclist[128];
	int threadid;
}MemEnd_t;

void set_magiclist(unsigned char *magiclist, int cnt)
{
	int i;
	for (i=0;i<cnt;i++)
	{
		magiclist[i] = i;
	}
}

int check_magiclist(unsigned char *magiclist, int cnt)
{
	int i;
	for (i=0;i<cnt;i++)
	{
		if (i != magiclist[i])
			return -1;
	}
	return 0;
}

void *__wrap_malloc(size_t c)
{
	unsigned char *ptr;
	size_t rs;
	rs = c + sizeof(MemStart_t) + sizeof(MemEnd_t);

	ptr = (unsigned char *)__real_malloc(rs);
	if (!ptr)
	{
		printf("ERROR: Failed malloc memory, rs: %d\n", rs);
	}
	if (1)
	{
		MemStart_t start;
		MemEnd_t end;
		start.magic = MEMTEST_MAGIC;
		start.size = c;
		start.threadid = 0;
		start.next = NULL;
		end.magic = MEMTEST_MAGIC;
		end.threadid = start.threadid;
		set_magiclist(start.magiclist, sizeof(start.magiclist));
		set_magiclist(end.magiclist, sizeof(end.magiclist));

		memcpy(ptr, &start, sizeof(MemStart_t));
		memcpy(ptr+c+sizeof(MemStart_t), &end, sizeof(MemEnd_t));
//printf("after malloc: %d, ptr: %p, size: %d\n", c, ptr, sizeof(MemStart_t));
		ptr += sizeof(MemStart_t);
	}

	return ptr;
}

//extern "C" void break_function(void);

void __wrap_free(void * rptr)
{
	unsigned char *ptr = (unsigned char *)rptr;
	if (1)
	{
		size_t c;
		MemStart_t start;
		MemEnd_t end;

		ptr -= sizeof(MemStart_t);
		memcpy(&start, ptr, sizeof(MemStart_t));
		if (start.magic == MEMTEST_MAGIC)
		{
			memset(ptr, 0, sizeof(MemStart_t));
			c = start.size;
			memcpy(&end, ptr+c+sizeof(MemStart_t), sizeof(MemEnd_t));
		}

		if (start.magic != MEMTEST_MAGIC
				|| end.magic != MEMTEST_MAGIC
				|| 0 != check_magiclist(start.magiclist, sizeof(start.magiclist))
				|| 0 != check_magiclist(end.magiclist, sizeof(end.magiclist))
				)
		{
			printf("ERROR: memory is destroied: start: %x, end: %x, size: %d, ptr: %p\n", start.magic, end.magic, start.size, ptr);
			ptr += sizeof(MemStart_t) ;
//			break_function();
		}
		else
			memset(ptr+c+sizeof(MemStart_t), 0, sizeof(MemEnd_t));
	}
//printf("before free: %p\n", ptr);
	__real_free(ptr);
//	printf("end free\n");
}

void *__wrap_realloc(void *rptr,size_t size)
{
//	printf("realloc running\n");
	unsigned char *ptr = (unsigned char *)rptr;
	unsigned char *res = (unsigned char *)malloc(size);

	if (0)
	{
		MemStart_t start;
		memcpy(&start, ptr-sizeof(MemStart_t), sizeof(MemStart_t));
		if (start.magic == MEMTEST_MAGIC)
		{
			if (size > start.size)
			{
				size = start.size;
			}
		}
	}
	if (ptr)
	{
		memcpy(res, ptr, size);
		free(ptr);
	}
//	printf("realloc end\n");
	return res;
	//return __real_realloc(ptr,size);
}

extern "C" void *__wrap_calloc(size_t nmemb,size_t size)
{
        char* p = (char*)malloc(nmemb*size);
	memset(p,0,size*nmemb);
	return p;
}

