

/*	mprivacy.h
	Copyright (C) 2011 Jovision Technology Co., Ltd.
	此文件用来组织隐私区域遮挡功能相关函数
	更改历史详见svn版本库日志
*/
#ifndef __MPRIVACY_H__
#define __MPRIVACY_H__

#define MAX_PYRGN_NUM		8		//遮挡区域个数privacy

//视频遮挡区域
typedef struct tagREGION
{
	BOOL	bEnable;
	RECT	stRect[MAX_PYRGN_NUM];
}REGION, *PREGION;

/**
 *@brief 初始化
 *@return 0 或者错误号
 *
 */
int mprivacy_init(void);

/**
 *@brief 结束
 *@return 0 或者错误号
 *
 */
int mprivacy_deinit(void);

/*
 * @brief 停止
 */
int mprivacy_stop(int channelid);

/*
 * @brief 开始
 */
int mprivacy_start(int channelid);

/**
 *@brief 获取参数
 *@param channelid 通道号
 *@param region 通道参数
 *
 *@return 0 或者错误号
 *
 */
int mprivacy_get_param(int channelid, REGION *region);

/**
 *@brief 设置参数
 *@param channelid 通道号
 *@param region 通道参数
 *
 *@return 0 或者错误号
 *
 */
int mprivacy_set_param(int channelid, REGION *region);

/**
 *@brief 使设置生效
 *@param channelid 通道号
 *
 *@return 0 或者错误号
 *
 */
int mprivacy_flush(int channelid);


#endif

