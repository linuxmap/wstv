/*
 * mvoicedec.h
 *
 *  Created on: 2014年9月22日
 *      Author: lfx
 *  本文件用来解析音频设置WIFI的功能
 */

#ifndef MVOICEDEC_H_
#define MVOICEDEC_H_

/*音频输入输出切换到24k采样率 来使能声波配置WIFI功能，这时声音不能被编码*/
int mvoicedec_enable(void);

/*音频输入输出切换到8k采样率 来禁止声波配置WIFI功能，这时声音可以被编码*/
int mvoicedec_disable(void);

int mvoicedec_init(void);
//开启计时，计时值发送到广播中，手机端通过这个值确认设备
int Start_timer_for_count();

BOOL CheckbAppendSearchPrivateInfo();

BOOL isVoiceSetting();

//开启声波后，是否收到网络配置信息
BOOL isVoiceRecvAndSetting();


#endif /* MVOICEDEC_H_ */
