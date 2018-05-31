#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <stdlib.h>
#include "mem_debug.h"
#include "os_wrapper.h"


MEM_API void *__real_malloc(size_t);
MEM_API void *__real_calloc(size_t nmemb,size_t);
MEM_API void *__real_realloc(void* ptr,size_t size);
MEM_API void __real_free(void*);


typedef struct
{
	int		ThreadId;
	char	ThreadName[TASK_NAMELEN];
	char*	pAddr;
	size_t	nSize;
	int		RegLR;
}MemInfo;

enum
{
	ReqType_Alloc,
	ReqType_Free,
};

typedef struct
{
	MemInfo Info;
	int		ReqType;
}MemRequest;

typedef struct
{
	int		tid;
	char	ThreadName[TASK_NAMELEN];
	size_t	nCnt;
	size_t	nSize;
}ThreadMemInfo;

#define MEM_BLOCK_CNT		4096
#define THREAD_MAX_CNT		256
#define THREAD_ID_MAX		4096


static bool			s_Enable = false;
static int			s_MinMemLimit = 1;

typedef struct queue_node
{
	struct queue_node*	next;
	MemRequest			Req;
}queue_node;

typedef struct
{
	queue_node*	head;
	queue_node*	tail;
	int			cnt;
}queue;

static queue	s_RequstQueue = {NULL, NULL, 0};
static pthread_mutex_t s_QueueLock = PTHREAD_MUTEX_INITIALIZER;
static int		s_QueueMaxCnt = 0;


static MEM_API void queue_post(const MemRequest* pReq)
{
	queue_node* pnode = __real_calloc(1, sizeof(queue_node));

	pnode->Req = *pReq;
	pnode->next = NULL;

	pthread_mutex_lock(&s_QueueLock);
	if (!s_RequstQueue.head)
		s_RequstQueue.head = s_RequstQueue.tail = pnode;
	else
	{
		s_RequstQueue.tail->next = pnode;
		s_RequstQueue.tail = s_RequstQueue.tail->next;
	}
	++s_RequstQueue.cnt;
	if (s_QueueMaxCnt < s_RequstQueue.cnt)
		s_QueueMaxCnt = s_RequstQueue.cnt;
	pthread_mutex_unlock(&s_QueueLock);
}

static MEM_API int queue_recv(MemRequest* pReq)
{
	if (!pReq)		return -1;

	queue_node* pHead = NULL;

	pthread_mutex_lock(&s_QueueLock);
	if (s_RequstQueue.head)
	{
		pHead = s_RequstQueue.head;
		s_RequstQueue.head = pHead->next;
		if (!s_RequstQueue.head)
			s_RequstQueue.tail = NULL;
		--s_RequstQueue.cnt;
	}
	pthread_mutex_unlock(&s_QueueLock);

	if (!pHead)		return -2;

	*pReq = pHead->Req;
	__real_free(pHead);

	return 0;
}

MEM_API void mem_add_info(char* p, size_t nSize, int lr)
{
	if (!s_Enable)
		return;
	if (!p || nSize < s_MinMemLimit)
		return;

	MemRequest Req = {{0}};
	Req.ReqType = ReqType_Alloc;
	Req.Info.nSize = nSize;
	Req.Info.pAddr = p;
	Req.Info.RegLR = lr;
	Req.Info.ThreadId = os_getpid();
	os_get_thread_name(Req.Info.ThreadId, Req.Info.ThreadName);

	queue_post(&Req);
}

MEM_API void mem_del_info(char* p)
{
	if (!s_Enable)
		return;
	if (!p)
		return;

	MemRequest Req = {{0}};
	Req.ReqType = ReqType_Free;
	Req.Info.pAddr = p;

	queue_post(&Req);
}

typedef struct list_node
{
	struct list_node*	next;
	MemInfo				Info;
}list_node;

typedef struct
{
	list_node*	head;
	int			cnt;
}mem_list;


static mem_list		s_MemList;
static pthread_mutex_t s_MemListLock = PTHREAD_MUTEX_INITIALIZER;
static int			s_ListMaxCnt = 0;
static pthread_t	s_WorkThreadId;

static MEM_API void list_add_head(const MemInfo* pInfo)
{
	list_node* pnode = __real_calloc(1, sizeof(list_node));

	pnode->Info = *pInfo;
	pnode->next = NULL;

	pthread_mutex_lock(&s_MemListLock);
	pnode->next = s_MemList.head;
	s_MemList.head = pnode;
	++s_MemList.cnt;
	if (s_ListMaxCnt < s_MemList.cnt)
		s_ListMaxCnt = s_MemList.cnt;
	pthread_mutex_unlock(&s_MemListLock);
}

static MEM_API int list_remove(const char* pAddr)
{
	list_node* pHead = NULL;
	list_node* pDel = NULL;

	pthread_mutex_lock(&s_MemListLock);
	if (s_MemList.head)
	{
		if (s_MemList.head->Info.pAddr == pAddr)
		{
			pDel = s_MemList.head;
			s_MemList.head = s_MemList.head->next;
			--s_MemList.cnt;
		}
		else
		{
			pHead = s_MemList.head;
			while (pHead->next != NULL)
			{
				if (pHead->next->Info.pAddr == pAddr)
				{
					pDel = pHead->next;
					pHead->next = pHead->next->next;
					--s_MemList.cnt;
					break;
				}
				pHead = pHead->next;
			}
		}
	}
	pthread_mutex_unlock(&s_MemListLock);

	if (!pDel)		return -2;

	__real_free(pDel);

	return 0;
}

static void* __mem_work_thread(void* p)
{
	prctl(PR_SET_NAME, __func__);

	MemRequest Req = {{0}};

	// 提高优先级，避免请求队列积压，占用过多内存
	// LOS_CurTaskPriSet(6);

	while (1)
	{
		while(queue_recv(&Req) == 0)
		{
			switch (Req.ReqType)
			{
			case ReqType_Alloc:
				list_add_head(&Req.Info);
				break;
			case ReqType_Free:
				list_remove(Req.Info.pAddr);
				break;
			default:
				printf("Error, __mem_work_thread\n");
				break;
			}
		}
		usleep(1*1000);
	}

	return NULL;
}

MEM_API void StartMemCheck(int MinMemLimit)
{
	s_Enable = true;
	if (MinMemLimit >= 1)
	{
		s_MinMemLimit = MinMemLimit;
	}
	if (!s_WorkThreadId)
	{
		pthread_create(&s_WorkThreadId, NULL, __mem_work_thread, NULL);
	}
	printf("Memory check started!! MinLimit: %d\n", s_MinMemLimit);
}

MEM_API void StopMemCheck(void)
{
	s_Enable = false;
	printf("Memory check stopped!!\n");
}

MEM_API void ClearMemCheck(void)
{
	printf("Memory clear failed, not implement!!\n");
}

MEM_API void ListMemInfo(int tid, int minlimit)
{
	int cnt = 0;
	int total = 0;
	int i = 0;
	int list_cnt = 0;
	list_node* pNode = NULL;
	MemInfo* pInfo = NULL;
	MemInfo* pItr = NULL;

	pthread_mutex_lock(&s_MemListLock);
	pInfo = __real_calloc(s_MemList.cnt, sizeof(MemInfo));
	if (!pInfo)
	{
		pthread_mutex_unlock(&s_MemListLock);
		printf("malloc failed!\n");
		return;
	}
	pItr = pInfo;
	pNode = s_MemList.head;
	while (pNode)
	{
		if ((pNode->Info.ThreadId == tid) || (-1 == tid))
		{
			if ((int)pNode->Info.nSize >= minlimit)
			{
				*pItr = pNode->Info;
				++cnt;
				++pItr;
			}
		}
		pNode = pNode->next;
	}
	list_cnt = s_MemList.cnt;
	pthread_mutex_unlock(&s_MemListLock);

	printf("....................................................................\n");
	printf(" Index    Size  Addr          Thread  LR          Name\n");

	pItr = pInfo;
	for (i = 0; i < cnt; ++i)
	{
		printf("%6d  %6d  0x%08x    %-4d    0x%08x  %s\n",
				i, pItr->nSize, (int)pItr->pAddr, pItr->ThreadId, pItr->RegLR, pItr->ThreadName);
		total += pItr->nSize;
		++pItr;
	}

	__real_free(pInfo);

	printf("....................................................................\n");
	printf("..........total: %4d         rs: %6d B(%d KB/%.3f MB)..........\n", cnt, total, total >> 10, total / 1024.0 / 1024);
	printf("..........queue cnt: %d, max: %d..........\n", s_RequstQueue.cnt, s_QueueMaxCnt);
	printf("..........list cnt: %d, max: %d..........\n", list_cnt, s_ListMaxCnt);
}

MEM_API void ListThreadMemInfo(void)
{
	int cnt = 0;
	int total = 0;
	int list_cnt = 0;
	int queue_cnt = s_RequstQueue.cnt;
	ThreadMemInfo ThMemInfo[THREAD_MAX_CNT];
	char TidMap[THREAD_ID_MAX];
	list_node* pNode = NULL;
	int Index = 0;
	int i = 0;

	memset(&ThMemInfo, 0, sizeof(ThMemInfo));
	memset(TidMap, 0xFF, sizeof(TidMap));

	pthread_mutex_lock(&s_MemListLock);
	pNode = s_MemList.head;
	while (pNode)
	{
		++cnt;
		total += pNode->Info.nSize;

		if (pNode->Info.ThreadId >= THREAD_ID_MAX)
		{
			printf("Invalid threadid: %d, max: %d\n", pNode->Info.ThreadId, THREAD_ID_MAX);
		}

		if (TidMap[pNode->Info.ThreadId] == (char)-1)
		{
			TidMap[pNode->Info.ThreadId] = i;
			++i;
		}
		Index = (unsigned int)TidMap[pNode->Info.ThreadId];
		ThMemInfo[Index].tid = pNode->Info.ThreadId;
		++ThMemInfo[Index].nCnt;
		ThMemInfo[Index].nSize += pNode->Info.nSize;
		if (!ThMemInfo[Index].ThreadName[0])
			strcpy(ThMemInfo[Index].ThreadName, pNode->Info.ThreadName);

		pNode = pNode->next;
	}
	list_cnt = s_MemList.cnt;
	pthread_mutex_unlock(&s_MemListLock);

	printf("....................................................................\n");
	printf("  tid  Name                            Count       Size\n");
	for (i = 0; i < THREAD_MAX_CNT; i++)
	{
		if (!ThMemInfo[i].nCnt)
			continue;
		
		printf("%4d  %-30s  %-4d    %8d / %9.2f / %9.2f\n",
				ThMemInfo[i].tid, ThMemInfo[i].ThreadName, ThMemInfo[i].nCnt, ThMemInfo[i].nSize, ThMemInfo[i].nSize / 1024.0, ThMemInfo[i].nSize / (1024.0 * 1024));
	}
	printf("....................................................................\n");
	printf("..........total: %4d         rs: %6d B(%d KB/%.3f MB)..........\n", cnt, total, total >> 10, total / 1024.0 / 1024);
	printf("..........queue cnt: %d, max: %d..........\n", queue_cnt, s_QueueMaxCnt);
	printf("..........list cnt: %d, max: %d..........\n", list_cnt, s_ListMaxCnt);
}


static int lmem(int argc, char* argv[])
{
	int tid = -1;
	int minlimit = -1;
	--argc;++argv;
	if (argc > 0)
	{
		tid = atoi(argv[0]);
		if (argc > 1)
		{
			minlimit = atoi(argv[1]);
		}
	}
	ListMemInfo(tid, minlimit);
	return 0;
}
static int lthm(int argc, char* argv[])
{
	ListThreadMemInfo();
	return 0;
}
static int startm(int argc, char* argv[])
{
	int minlimit = -1;
	--argc;++argv;
	if (argc > 0)
	{
		minlimit = atoi(argv[0]);
	}
	StartMemCheck(minlimit);
	return 0;
}
static int stopm(int argc, char* argv[])
{
	StopMemCheck();
	return 0;
}
static int clearm(int argc, char* argv[])
{
	ClearMemCheck();
	return 0;
}

#if 0
#include "shell.h"

MEM_API void RegMemCmd(void)
{
	osCmdReg(CMD_TYPE_EX, "lmem", 1, lmem);
	osCmdReg(CMD_TYPE_EX, "lthm", 1, lthm);
	osCmdReg(CMD_TYPE_EX, "startm", 1, startm);
	osCmdReg(CMD_TYPE_EX, "stopm", 1, stopm);
	osCmdReg(CMD_TYPE_EX, "clearm", 1, clearm);

	char* argv[] = {"256"};
	startm(sizeof(argv) / sizeof(argv[0]), argv);
}
#else
#include "utl_cmd.h"
MEM_API void RegMemCmd(void)
{
	utl_cmd_insert("lmem", "list all memory block", "list all memory block", lmem);
	utl_cmd_insert("lthm", "list memory block of thread", "list memory block of thread", lthm);
	utl_cmd_insert("startm", "start memroy analyze", "start memroy analyze", startm);
	utl_cmd_insert("stopm", "stop memroy analyze", "stop memroy analyze", stopm);
	utl_cmd_insert("clearm", "clear all memory block", "clear all memory block", clearm);

	char* argv[] = {"startm", "0"};
	startm(sizeof(argv) / sizeof(argv[0]), argv);
}
#endif
