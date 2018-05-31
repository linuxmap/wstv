
/**
 *@file jv_smart_sensor.h file about judge light by sensor-self
 *@author lyl
 */
#ifndef _JV_SMART_SENSOR_H_
#define _JV_SMART_SENSOR_H_
typedef void (*jv_msensor_setcut_callback_t)(int param);
typedef int (*jv_msensor_obslog_callback_t)(char* report, char *log, ...);
int jv_msensor_set_obslog_callback(jv_msensor_obslog_callback_t callback);

/**
 *@brief 软光敏2.0初始化
 *@param	sensorid 比如 SENSOR_OV2735 ...
 */
int jv_sensor_smart_init(int sensorid);

/**
 *@brief 软光敏2.0日夜判断
 *@param	bNight当前日夜模式，index 日夜切换时刻等级，adcv adc读值
 */
int jv_sensor_smart_judge(int bNight, unsigned int index, unsigned int adcv);


int jv_sensor_ircut_check(int sensorid, int nightmode, jv_msensor_setcut_callback_t setcut);
#endif

