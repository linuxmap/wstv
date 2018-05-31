/**
 *@file jv_autotrack.h motion detection about
 * define the interface of motion detection
 *@author SunYingLiang
 */

#ifndef _JV_MOVETRACK_H_
#define _JV_MOVETRACK_H_

#include "jv_common.h"
//IVE取数据的宽高
//#define ATK_VI_W	352		
//#define ATK_VI_H	288
#define ATK_VI_W	160		
#define ATK_VI_H	120


//#define AT_MMZ_CACHE 1

#define DUMP_FILE 1
#define MIN_OBJ_SIZE  4
#define OBJ_MERGE_LIMIT     1                            //认为相邻连通域是同一物体的限制
#define OBJ_MOTION_THESAME_LIMIT   0		//认为运动中同一物体分离度的限制
#ifndef IVE_MAX_REGION_NUM
#define IVE_MAX_REGION_NUM 254
#endif
typedef struct
{
	unsigned int	x;
	unsigned int	y;
	unsigned int	w;
	unsigned int	h;
	unsigned int	label;
}RECT_t;
typedef struct
{
	int x;
	int y;
	int w;
	int h;
	int left;
	int right;
	int top;
	int bottom;
	int vector_x;
	int vector_y;
	int x_buf[10];
	int y_buf[10];
	int top_buf[10];
	int bottom_buf[10];
	int x_begin;
	int y_begin;
	int w_begin;
	int h_begin;
	int left_begin;
	int right_begin;
	int top_begin;
	int bottom_begin;
	int area;
	int lable;		//区分物体的标签
	int repeats;	//连续出现次数
	int IntersectCounts;	//和计数视频画线的相交次数
	int IntersectCountsClimb;	//和攀爬视频画线的相交次数

	BOOL bCounted;		//是否已经加过了
	int 	matchrule;
	int 	staytimeouts;
	U64 	  lasttime;
	BOOL bAlarmed;


} ObjStaticsInfo_t;

typedef struct
{
//	ObjStaticsInfo_t ObjStatics[IVE_MAX_REGION_NUM];
//	int objNum;
	BOOL bIveProcRuning;
	jv_thread_group_t group;

	
	
} stMVAInfo_t;

//遮挡分窗口
#define MAXWIN_W	8
#define MAXWIN_H	8

//遮挡统计值
typedef struct
{
	int nWinW;
	int nWinH;
	int threshold;
	int statics[MAXWIN_W][MAXWIN_H];

} ObjHideStatics_t;

//内联加快速度，目前用的地方还算少一些
static inline void ccl_merge( ObjStaticsInfo_t * sObj,  ObjStaticsInfo_t * sArray)
{
//		printf("CCl   Merge \n ");
		ObjStaticsInfo_t  tmp;
		if(sObj->repeats > sArray->repeats)
			memcpy(&tmp, (void*)sObj,sizeof(ObjStaticsInfo_t));
		else
			memcpy(&tmp,(void*)sArray,sizeof(ObjStaticsInfo_t));

		if(sObj->left > sArray->left)
		{
			tmp.left = sArray->left;
		}
		else
		{
			tmp.left = sObj->left;
		}
		if(sObj->right < sArray->right)
		{
			tmp.right = sArray->right;
		}
		else
		{
			tmp.right = sObj->right;
		}
		
		if(sObj->bottom < sArray->bottom)
		{
			tmp.bottom = sArray->bottom;
		}
		else
		{

			tmp.bottom = sObj->bottom;
		}

		if(sObj->top > sArray->top)
		{
			tmp.top = sArray->top;
		}
		else
		{
			tmp.top = sObj->top;
		}
		
		if(sObj->repeats > sArray->repeats)
		{

			tmp.repeats = sObj->repeats;
		}
		else
		{

			tmp.repeats = sArray->repeats;

		}
		if(abs(sObj->vector_y) > abs(sArray->vector_y))
		{
			tmp.vector_y = sObj->vector_y;
		}
		else
		{
			tmp.vector_y = sArray->vector_y;

		}
		if(abs(sObj->vector_x) > abs(sArray->vector_x))
		{
			tmp.vector_x = sObj->vector_x;
		}
		else
		{
			tmp.vector_x = sArray->vector_x;

		}

		if(abs(sObj->bCounted) > abs(sArray->bCounted))
		{
			tmp.bCounted = sObj->bCounted;
		}
		else
		{
			tmp.bCounted = sArray->bCounted;

		}

		if(abs(sObj->IntersectCounts) > abs(sArray->IntersectCounts))
		{
			tmp.IntersectCounts = sObj->IntersectCounts;
		}
		else
		{
			tmp.IntersectCounts = sArray->IntersectCounts;

		}

		memcpy(sObj,&tmp,sizeof(ObjStaticsInfo_t));
		memcpy(sArray,&tmp,sizeof(ObjStaticsInfo_t));

}

static inline BOOL  __bNearObjs(ObjStaticsInfo_t *pStObj1 ,ObjStaticsInfo_t *pStObj2,int threshhold)
{
	int disx,disy;
	int lx,ly;
	disx = abs (((pStObj1->left + pStObj1->right)) - ((pStObj2->left+pStObj2->right))) ;
	disy = abs (((pStObj1->bottom + pStObj1->top)) - ((pStObj2->bottom+pStObj2->top))) ;
	lx = abs ((pStObj1->right - pStObj1->left) + (pStObj2->right - pStObj2->left)) + threshhold;
	ly=  abs ((pStObj1->bottom - pStObj1->top) + (pStObj2->bottom - pStObj2->top)) +threshhold ;
	if(disx <= lx && disy<=ly)
		return 1;
	return 0;
}


/**
 *@brief do initialize 
 *@return 0 if success.
 *
 */
 int jv_mva_init(void);
/**
 *@brief do de-initialize 
 *@return 0 if success.
 *
 */
int jv_mva_deinit(void);


/**
 *@brief start motion track
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@return 0 if success.
 *
 */
int jv_mva_start(int channelid);


/**
 *@brief stop 
 *@param channelid The id of the channel, It is always 0 in ipcamera.
 *@return 0 if success.
 *
 */
int jv_mva_stop(int channelid);

//连通域分析
typedef int (*jv_mva_ccl_analysis)(ObjStaticsInfo_t *stObj, int * ObjNum);
//设置连通域分析的回调
int  jv_mva_register_ccl_analysis_callback(jv_mva_ccl_analysis callback);
//检查挂起计算的回调
typedef BOOL  (*jv_mva_getsuspends)(void);
//设置检查计算的回调
int  jv_mva_register_getsuspends_callback(jv_mva_getsuspends callback);

//连通域分析
typedef int (*jv_mva_hide_analysis)(ObjHideStatics_t *stHide);
//设置连通域分析的回调
int  jv_mva_register_hide_analysis_callback(jv_mva_hide_analysis callback);

#endif
 
