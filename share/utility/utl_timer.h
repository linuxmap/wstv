#ifndef _MTIMER_H_
#define _MTIMER_H_
#include "jv_common.h"

/**
 *@brief timer callback type definition
 *@retval TRUE timer will run continue
 *@retval FALSE timer will stop.
 *
 */
int schedule_id[20];//定时任务

typedef BOOL (*utl_timer_callback_t)(int id, void* param);

typedef BOOL (*utl_schedule_callback_t)(int id,int second ,void* param);


/**
 *@brief timer init
 *@return 0 if success
 *
 */
int utl_timer_init(void);

/**
 *@brief timer deinit
 *@return 0 if success
 *
 */
int utl_timer_deinit(void);

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
int utl_timer_create(char *name, int millisecond, utl_timer_callback_t callback, void *param);


int utl_schedule_create(char *name, utl_schedule_callback_t callback, void *param);


int utl_schedule_Enable(int i, int second);


int utl_schedule_disable(int i);

/**
 *@brief reset the timer
 *@param id retval of #utl_timer_create
 *@param millisecond the new time to delay
 *@return 0 if success
 *
 */
int utl_timer_reset(int id, int millisecond, utl_timer_callback_t callback, void *param);

/**
 *@brief delete the timer
 *@param id retval of #utl_timer_create
 *@retval <0 if failed
 *@retval >=0 id of timer
 *
 */
int utl_timer_destroy(int id);

/**
 *@brief get microseconds after last run of this func
 */
unsigned int utl_timer_passed_microseconds();

#endif

