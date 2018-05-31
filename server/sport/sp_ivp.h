/*
 * sp_ivp.h
 *
 *  Created on: 2015-04-02
 *      Author: Qin Lichao
 */

#ifndef SP_IVP_H_
#define SP_IVP_H_


#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int x;
    int y;
}SPIVP_POINT;

typedef struct
{
	int						nCnt;						//点的个数>=2,=2时是绊线
	SPIVP_POINT				stPoints[10];	//图形的点的坐标
	unsigned int			nIvpCheckMode;				//方向检测
}SPIVPREGION_t;

typedef enum{
	SPCOUNTOSD_POS_LEFT_TOP=0,
	SPCOUNTOSD_POS_LEFT_BOTTOM,
	SPCOUNTOSD_POS_RIGHT_TOP,
	SPCOUNTOSD_POS_RIGHT_BOTTOM,
	SPCOUNTOSD_POS_HIDE,
}SPivpcountosd_pos_e;

typedef struct{
	BOOL				bEnable;					//是否开启智能视频分析

	unsigned int 		nDelay;						//报警延时，小于此时间的多次报警只发送一次邮件，客户端不受此限制
	BOOL 				bStarting;					//是否正在报警,由于客户端报警会一直持续，用来检测是否在向客户端发送报警

	unsigned int		nRgnCnt;					//报警区域个数
	SPIVPREGION_t		stRegion[4];//报警区域

	BOOL 				bDrawFrame;						//画出拌线或防区
    BOOL 				bFlushFrame;					//报警产生时拌线或防区边线闪烁
    BOOL				bMarkObject;					//标记报警物体--v2
	BOOL 				bMarkAll;						//标记全部运动物体

	BOOL				bOpenCount;						//开启人数统计功能--v2
	BOOL				bShowCount;						//显示人数统计--v2
	BOOL				bPlateSnap;						//车辆抓拍模式，开启后标记全部物体无效且只能划拌线--v2

	unsigned int		nAlpha;						//防区透明度
	unsigned int		nSen;						//灵敏度
	unsigned int		nThreshold;					//阀值
	unsigned int		nStayTime;					//停留时间

	BOOL 				bEnableRecord;				//是否开启报警录像
	BOOL 				bOutAlarm1;					//是否输出到 1路报警输出
	BOOL				bOutClient;					//是否输出到客户端报警
	BOOL				bOutEMail;					//是否发送邮件报警
	BOOL				bOutVMS;					//是否发送至VMS服务器

	BOOL				bNeedRestart;				//是否需要重启

	SPivpcountosd_pos_e	eCountOSDPos;					//位置
	unsigned int		nCountOSDColor;					//字体颜色

	unsigned int		nCountSaveDays;					//保存天数
	char				sSnapRes[16];
}SPIVP_t;

/**
 *@brief 布防
 *
 *@param channelid 智能分析通道
 */
int sp_ivp_start(int channelid);

/**
 *@brief 撤防
 *
 *@param channelid 智能分析通道
 */
int sp_ivp_stop(int channelid);

/**
 * @brief 获取智能分析报警信息
 *
 *@param channelid 智能分析通道
 * @param param 智能分析报警参数
 *
 * @return 0 成功
 */
int sp_ivp_get_param(int channelid, SPIVP_t *param);

/**
 * @brief 获取智能分析报警信息
 *
 *@param channelid 智能分析通道
 * @param param 智能分析报警参数
 *
 * @return 0 成功
 */
int sp_ivp_set_param(int channelid, SPIVP_t *param);


#ifdef __cplusplus
}
#endif

#endif /* SP_IVP_H_ */
