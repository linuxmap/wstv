#ifndef _UTL_COMMON_H_
#define _UTL_COMMON_H_


#define utl_WaitTimeout(flag, time_ms)			do\
												{\
													int n = ((time_ms) + 99) / 100;\
													while (!(flag) && n--)\
														utl_sleep_ms(100);\
												} while(0)
														

//获取系统运行的时间，单位毫秒。不会受到修改时间的影响
unsigned long long utl_get_ms();

unsigned int utl_get_sec();

//睡眠ms
void utl_sleep_ms(unsigned long ms);

// 获取当前时区和UTC时区相差的秒数
short utl_GetTZDiffSec();

//时间格式的相关操作
typedef enum
{
	UTL_TIME_HHMMSS,
} TimeType_e;

/**
  *@brief	修改对应时间格式时间 
  *@param	type 时间格式
  *@param	value 秒数(正负)
  *@return  返回对应格式的修改后的时间, <0 错误  
  */
int utl_time_modify(TimeType_e type, int time, int value);

/**
  *@brief	计算对应时间格式的时间差 
  *@param	type 时间格式
  *@param	stime 开始时间
  *@param	etime 结束时间
  *@return  返回对应时间相差的秒数
  */
int utl_time_range(TimeType_e type, int stime, int etime);

int utl_get_file_size(const char* filename);

int utl_copy_file(const char* srcfile, const char* dstfile);

#endif
