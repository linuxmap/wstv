
/*	mdetect.h
	Copyright (C) 2011 Jovision Technology Co., Ltd.
	此文件用来组织移动检测相关代码
	更改历史详见svn版本库日志
*/

#ifndef _MDETECT_H_
#define _MDETECT_H_

/**
 *@brief 需要报警时的回调函数
 * 该函数主要用于通知分控，是否有警报发生
 *@param channelid 发生报警的通道号
 *@param bAlarmOn 报警开启或者关闭
 *
 */
typedef void (*alarming_callback_t)(int channelid, BOOL bAlarmOn);
#define MAX_REGION_ROW 32
typedef struct tagMD
{
	BOOL	bEnable;			//是否开启移动检测
	U32		nSensitivity;		//灵敏度
	U32		nThreshold;			//移动检测阈值
	U32		nRectNum;			//移动检测区域个数，最大为4，0表示全画面检测
	RECT	stRect[MAX_MDRGN_NUM];
	BOOL 	bEnableRecord;		//是否开启报警录像

	U32		nDelay;
	U32		nStart;
	BOOL	bOutClient;			//是否发送至分控
	BOOL	bOutEMail;			//是否发送邮件
	BOOL	bOutVMS;			//是否发送至VMS服务器
	BOOL	bBuzzing;			//是否蜂鸣器报警

	U32		nRegion[MAX_REGION_ROW];		//移动侦测范围块
	int		nRow;				//行数
	int 	nColumn;			//列数
}MD, *PMD;

/**
 *@brief 初始化
 *@return 0
 *
 */
int mdetect_init(void);

/**
 *@brief 结束
 *@return 0
 *
 */
int mdetect_deinit(void);


/**
 *@brief 临时启用和禁用
 *@return 0
 *
 */
void mdetect_enable();

void mdetect_disable();

/**
 *@brief 设置报警回调函数
 *
 *
 */
int mdetect_set_callback(alarming_callback_t callback);

/**
 *@brief 设置参数
 *@param channelid 频道号
 *@param md 要设置的属性结构体
 *@note 如果不能确定所有属性的值，请先#mdetect_get_param获取原本的值
 *@return 0 成功，-1 失败
 *
 */
int mdetect_set_param(int channelid, MD *md);

/**
 *@brief 获取参数
 *@param channelid 频道号
 *@param md 要设置的属性结构体
 *@return 0 成功，-1 失败
 *
 */
int mdetect_get_param(int channelid, MD *md);

/**
 *@brief 使设置生效
 *	在#mdetect_set_param之后，如果改变了使能状态，调用本函数
 *@param channelid 频道号
 *@return 0 成功，-1 失败
 *
 */
int mdetect_flush(int channelid);

/**
 *@brief 开始移动检测
 *@param channelid 频道号
 *@return 0 成功，-1 失败
 *
 */
int mdetect_start(int channelid);

/**
 *@brief 停止移动检测
 *@param channelid 频道号
 *@return 0 成功，-1 失败
 *
 */
int mdetect_stop(int channelid);

/**
 *@brief 是否发生移动检测报警
 *@param channelid 频道号
 *@return 为真，则有报警发生。否则没有。
 *
 *@note #MD::nDelay 的值，决定了报警后多长时间内，是处于报警中
 *
 */
BOOL mdetect_b_alarming(int channelid);

extern int md_timer[MAX_STREAM];
BOOL _mdetect_timer_callback(int tid, void *param);

#endif

