#include <jv_common.h>
#include "utl_timer.h"
#include "utl_cmd.h"
#include <malarmout.h>

#define MAX_TIMER_CNT 20
typedef enum{
	TIMER_STATUS_IDLE,
	TIMER_STATUS_RUNNING,
	TIMER_STATUS_STOPED,
	TIMER_STATUS_WAITING_DEL///< 等待删除
}timer_status_e;
typedef struct{
	char name[12];
	timer_status_e status;	
	int usecond;//超时时间
	utl_timer_callback_t callbackptr;
	void *param;
	int ucounted;//已经过的时间
}timer_info_t;

typedef struct{
	char name[12];
	timer_status_e status;	
	int second;//定时时间
	utl_schedule_callback_t callbackpse;
	void *param;
	int   en;//是否启动
	int   ucounted;
}schedule_info_t;


static timer_info_t ti[MAX_TIMER_CNT];
static schedule_info_t sc[MAX_TIMER_CNT];


static jv_thread_group_t timer_td;
static jv_thread_group_t schedule_td;


static void _timer_process(void)
{
	int i;
	int ticks = 100;
	BOOL ret;
	pthreadinfo_add((char *)__func__);
	while(timer_td.running)
	{
		usleep(ticks*1000);
		for (i=0;i<MAX_TIMER_CNT;i++)
		{
			if (ti[i].status == TIMER_STATUS_RUNNING && ti[i].usecond >= 0)//负值，则永不超时
			{
				ti[i].ucounted += ticks;
				if (ti[i].ucounted > ti[i].usecond)
				{
					ret = ti[i].callbackptr(i, ti[i].param);
					pthread_mutex_lock(&timer_td.mutex);
					if (ti[i].status == TIMER_STATUS_RUNNING)
					{
						//这说明，在回调期间，它被reset了，否则肯定不可能为0
						if (ti[i].ucounted == 0)
						{
						}
						else
						{
							ti[i].ucounted = 0;
							if (!ret)
								ti[i].status = TIMER_STATUS_STOPED;
						}
					}
					pthread_mutex_unlock(&timer_td.mutex);
				}
			}
			else if (ti[i].status == TIMER_STATUS_WAITING_DEL)
			{
				ti[i].status = TIMER_STATUS_IDLE;
			}
		}
	}
}

static void _schedule_process(void)
{
	int i;
	int ticks = 100;
	BOOL ret;
	pthreadinfo_add((char *)__func__);
	int pre; 		//时间修改时不会抓拍和发送邮件
	while(schedule_td.running)
	{
		time_t tt;
		struct tm tm;
		tt = time(NULL);
		localtime_r(&tt, &tm);
		int now = tm.tm_hour*60 + tm.tm_min;
		for (i=0;i<MAX_TIMER_CNT;i++)
		{
			if (sc[i].status == TIMER_STATUS_RUNNING&&sc[i].en==1)
			{
				if ((now - sc[i].second == 0) &&sc[i].ucounted==0)
				{
					ret = sc[i].callbackpse(i,sc[i].second,sc[i].param);	
				}
				
				pthread_mutex_lock(&schedule_td.mutex);
				if( sc[i].second <= now )
				{
					if(now - pre > 1)
					{
						pthread_mutex_unlock(&schedule_td.mutex);
						malarm_flush();
						pthread_mutex_lock(&schedule_td.mutex);
					}
					else if(sc[i].second != 0 || (sc[i].second == now && now == 0))		//0点定时不置一
						sc[i].ucounted=1;
				}
				else
				{
					 sc[i].ucounted=0;
				}
				pthread_mutex_unlock(&schedule_td.mutex);
			}
			else if (sc[i].status == TIMER_STATUS_WAITING_DEL)
			{
				sc[i].status = TIMER_STATUS_IDLE;
			}
		}
		pre = now;
		usleep(ticks*1000);
	}
}

static int _mtimer_help(int argc, char *argv[])
{
	int i;
	char *status[] = {"idle", "running", "stop", "wait del"};
	printf("   name       status    countted usecond  callback   param\n");
	for (i=0;i<MAX_TIMER_CNT;i++)
	{
		printf("%12s  %8s %8d %8d 0x%08x 0x%08x\n", ti[i].name, status[ti[i].status],  ti[i].ucounted, ti[i].usecond, (unsigned int)ti[i].callbackptr, (unsigned int)ti[i].param);
	}
	return 0;
}

/**
 *@brief timer init
 *@return 0 if success
 *
 */
int utl_timer_init(void)
{
	memset(ti, 0, MAX_TIMER_CNT*sizeof(timer_info_t));
	timer_td.running = TRUE;
	pthread_mutex_init(&timer_td.mutex, NULL);
	pthread_create_normal(&timer_td.thread, NULL, (void *)_timer_process, NULL);

	memset(sc, 0, MAX_TIMER_CNT*sizeof(schedule_info_t));//创建定时计划
	schedule_td.running = TRUE;
	pthread_mutex_init(&schedule_td.mutex, NULL);
	pthread_create_normal(&schedule_td.thread, NULL, (void *)_schedule_process, NULL);
	
	utl_cmd_insert("timer", "display all timer list", "", _mtimer_help);
	return 0;
}

/**
 *@brief timer deinit
 *@return 0 if success
 *
 */
int utl_timer_deinit(void)
{
	timer_td.running = FALSE;
	pthread_join(timer_td.thread, NULL);
	pthread_mutex_destroy(&timer_td.mutex);

	schedule_td.running = FALSE;
	pthread_join(schedule_td.thread, NULL);
	pthread_mutex_destroy(&schedule_td.mutex);
	return 0;
}


/**
 *@brief create one timer
 *@param millisecond time delay, with millisecond.
 *		if millisecond is negative, none timeout will occured
 *@param callback function ptr to call back
 *@param param param of callback 
 *@retval <0 if failed
 *@retval >=0 id of timer
 *
 */
int utl_schedule_create(char *name, utl_schedule_callback_t callback, void *param)
{
	int i;
	pthread_mutex_lock(&schedule_td.mutex);

	for (i=0;i<MAX_TIMER_CNT;i++)
	{
		if (sc[i].status == TIMER_STATUS_IDLE)
		{
			strncpy(sc[i].name, name, sizeof(sc[i].name)-1);
			sc[i].callbackpse = callback;
			sc[i].param = param;
			sc[i].ucounted = 0;
			sc[i].second = 0;
			sc[i].en=0; //使能并未开启
			sc[i].status = TIMER_STATUS_RUNNING;
			break;
		}
	}
		
	pthread_mutex_unlock(&schedule_td.mutex);
	if (i == MAX_TIMER_CNT)
	{
		Printf("No free timer to create now...\n");
		return -1;
	}
	return i;
}

int utl_schedule_Enable(int i, int second)
{
	pthread_mutex_lock(&schedule_td.mutex);

	sc[i].ucounted = 0;
	sc[i].en = 1;
	sc[i].second = second;	
	pthread_mutex_unlock(&schedule_td.mutex);
	
	return i;
}

int utl_schedule_disable(int i)
{
	pthread_mutex_lock(&schedule_td.mutex);

	sc[i].en = 0;
		
	pthread_mutex_unlock(&schedule_td.mutex);
	
	return i;
}


int utl_timer_create(char *name, int millisecond, utl_timer_callback_t callback, void *param)
{
	int i;
	pthread_mutex_lock(&timer_td.mutex);

	for (i=0;i<MAX_TIMER_CNT;i++)
	{
		if (ti[i].status == TIMER_STATUS_IDLE)
		{
			strncpy(ti[i].name, name, sizeof(ti[i].name)-1);
			ti[i].callbackptr = callback;
			ti[i].param = param;
			ti[i].ucounted = 0;
			ti[i].usecond = millisecond;
			ti[i].status = TIMER_STATUS_RUNNING;
			break;
		}
	}
		
	pthread_mutex_unlock(&timer_td.mutex);
	if (i == MAX_TIMER_CNT)
	{
		Printf("No free timer to create now...\n");
		return -1;
	}
	return i;
}


/**
 *@brief reset the timer
 *@param id retval of #utl_timer_create
 *@param usecond the new time to delay
 *@return 0 if success
 *
 */
int utl_timer_reset(int id, int usecond, utl_timer_callback_t callback, void *param)
{
	if (id < 0 || id > MAX_TIMER_CNT)
	{
		Printf("ERROR: Wrong id reseted: %d\n", id);
		return JVERR_BADPARAM;
	}
	pthread_mutex_lock(&timer_td.mutex);
	ti[id].callbackptr = callback;
	ti[id].param = param;
	ti[id].ucounted = 0;
	ti[id].usecond = usecond;
	ti[id].status = TIMER_STATUS_RUNNING;
	pthread_mutex_unlock(&timer_td.mutex);

	return 0;
}

/**
 *@brief delete the timer
 *@param id retval of #utl_timer_create
 *@retval <0 if failed
 *@retval >=0 id of timer
 *
 */
int utl_timer_destroy(int id)
{
	pthread_mutex_lock(&timer_td.mutex);
	ti[id].status = TIMER_STATUS_WAITING_DEL;
	pthread_mutex_unlock(&timer_td.mutex);
	return 0;
}

void utl_print_buffer(unsigned char *buffer, int len, int lineCnt)
{
	int i;
	int j;
	for (i=0,j=0;i<len;i++)
	{
		printf("%02x,", buffer[i]);
		j++;
		if (j == lineCnt)
		{
			j = 0;
			printf("\n");
		}
	}
}

/**
 *@brief get microseconds after last run of this func
 */
unsigned int utl_timer_passed_microseconds()
{
	static unsigned int oldms;
	unsigned int nowms;
	unsigned int ret;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	nowms = tv.tv_sec * 1000 + (tv.tv_usec/1000);
	ret = nowms - oldms;
	oldms = nowms;
	return ret;
}

