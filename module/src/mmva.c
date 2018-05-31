/**
 *@file		小维视频运动分析motion video analysis
 *@brief		过线计数、区域入侵越界侦测等
 *@author	Sunyingliang  125261147@qq.com
 */
 #include "hicommon.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jv_stream.h"
#include "mstream.h"
#include "mosd.h"
#include "utl_common.h"
#include "mivp.h"
#include "mmva.h"
#include "jv_mva.h"
#include "mptz.h"
#include "jv_osddrv.h"
#include "malarmout.h"
#include "utl_timer.h"

#define MINI_REPEAT_NUM  		8
#define MINI_SEPARATE_THRESHHOLD		 	10
#define OBJ_TIMEREGION_MERGE_LIMIT 	 	0
#define MINI_OBJ_REPEAT_TIMES			 	5
#define OSD_STAY_TIMEOUTS					60	//单位是帧 fps
#define ALARM_OSDFLICKER_KEEP_TIMEOUTS	4	//单位秒s

#define OSD_DISP_STEP_LENGTH				1
#define OSD_DISP_MIN_SIZE					2

#define MOTION_VECTOR_THRESHHOLD	 		8

#define DEFAULT_COLOR		 0x00FFA0
#define ALARM_COLOR		 0xFF0000
#define REGION_LINE_COLOR	 0xFFA000

#define  CLIMB_UP_COLOR	 	 0xFF00FF

#define  LINE_COUNT_COLOR	 0x0022FF


#define DELAYWORK_TIMEOUT	 100
static int delaywork = 0;
static int bInited = FALSE;
typedef struct
{
	int mode;
	int direct;
	int point_num;
	POINT_S point[2];
	
} stCheckRule_t;


typedef struct
{
	ObjStaticsInfo_t ObjStatics[IVE_MAX_REGION_NUM];
	int objNum;
	ObjStaticsInfo_t ObjFilterStatics[IVE_MAX_REGION_NUM];
	int objFilterNum;
	U32 count_in;
	U32 count_out;
	stCheckRule_t rule[1];
	U32 viW;
	U32 viH;
	U32 nSensitivity;
	U32 nAlarmtime;
	BOOL bHideAlarmStart;
	BOOL bLinecountSupport;
	BOOL bHideCheckSupport;
	BOOL bClimbCheckSupport;
	int mva_tid;
} stMMVA_t;

static MIVP_t stMivp;

stMMVA_t stMmva;

static  ON_IVP_ALARM  _mmva_alram=NULL;		// mivp报警回调


//坐标复制
static void mmva_copy_pos(ObjStaticsInfo_t *dst ,ObjStaticsInfo_t *src)
{
	
	dst->left=src->left;
	dst->right=src->right;
	dst->top=src->top;
	dst->bottom=src->bottom;
	dst->w = src->w;
	dst->h = src->h;
	dst->x = src->x;
	dst->y = src->y;
}
//复制坐标到初始位置
static void mmva_copy_beginpos(ObjStaticsInfo_t *dst ,ObjStaticsInfo_t *src)
{
	
	dst->left_begin = src->left_begin;
	dst->right_begin = src->right_begin;
	dst->top_begin = src->top_begin;
	dst->bottom_begin =src->bottom_begin;
	dst->w_begin = src->w_begin;
	dst->h_begin  = src->h_begin;
	dst->x_begin = src->x_begin;
	dst->y_begin  = src->y_begin;
}
//初始化初始位置
static void mmva_copy_pos2beginpos(ObjStaticsInfo_t *dst ,ObjStaticsInfo_t *src)
{
	
	dst->left_begin = src->left;
	dst->right_begin = src->right;
	dst->top_begin = src->top;
	dst->bottom_begin =src->bottom;
	dst->w_begin = src->w;
	dst->h_begin  = src->h;
	dst->x_begin = src->x;
	dst->y_begin  = src->y;
}
//判断两个目标是不是可以合并成1个
static BOOL  mmva_judge_theSameObjs(ObjStaticsInfo_t *pStObj1 ,ObjStaticsInfo_t *pStObj2,int threshhold)
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



//判断两条直线相交
static BOOL mmva_judge_line_intersect(int ax,int ay,int bx,int by, int cx,int cy,int dx,int dy)
{
	if(MIN(ax,bx)<=MAX(cx,dx) && MIN(cy,dy)<=MAX(ay,by)&&MIN(cx,dx)<=MAX(ax,bx) && MIN(ay,by)<=MAX(cy,dy)) 
		return TRUE;

	return FALSE;

}
//判断直线和矩形相交,包含直线在矩形内部的情况
static BOOL mmva_judge_rectangle_and_line_intersect(int ax,int ay,int bx,int by,int left,int top, int right,int bottom,int numeratorW ,int denominatorW,int numeratorH ,int denominatorH)
{
	int lax, lay, lbx, lby;

	lax = ax*numeratorW/denominatorW;
	lay = ay*numeratorH/denominatorH;
	lbx = bx*numeratorW/denominatorW;
	lby = by*numeratorH/denominatorH;
	
	if(mmva_judge_line_intersect(lax,lay,lbx,lby,left,top,right,top ))
	{
		return TRUE;
	}
	else if(mmva_judge_line_intersect(lax,lay,lbx,lby,left,top,left,bottom ))
	{
		return TRUE;
	}
	else if(mmva_judge_line_intersect(lax,lay,lbx,lby,right,bottom,left,bottom ))
	{
		return TRUE;
	}
	else if(mmva_judge_line_intersect(lax,lay,lbx,lby,right,bottom,right,top ))
	{
		return TRUE;
	}
	else if((lax>= left) && (lax <= right) && (lay >= bottom) && (lay<= top) )
	{
		return TRUE;
	}
	else if((lbx>= left) && (lbx <= right) && (lby >= bottom) && (lby<= top) )
	{
		return TRUE;
	}

	return FALSE;

}
//判断点在多边形内部，不包含在边上
static BOOL mmva_judge_point_in_region(int ax,int ay,MIVPREGION_t 	*pStRegion, int numeratorW ,int denominatorW ,int numeratorH ,int denominatorH )
{
	int i;
	IVP_POINT p1,p2;
	int nCross=0;
//	printf("regin cnt=%d \n",pStRegion->nCnt);
	for(i=0; i<pStRegion->nCnt ;i++)
	{
		 p1.x =( numeratorW * pStRegion->stPoints[i].x )/denominatorW;	//坐标归一化到IVE输入
		 p1.y =( numeratorH * pStRegion->stPoints[i].y)/denominatorH;	//坐标归一化到IVE输入
		 
		 p2.x = (numeratorW * pStRegion->stPoints[(i+1)%(pStRegion->nCnt)].x)/denominatorW;
		 p2.y = (numeratorH * pStRegion->stPoints[(i+1)%(pStRegion->nCnt)].y)/denominatorH;
		 
		if ( p1.y == p2.y )
			continue; 
		if ( ay < MIN(p1.y, p2.y))
			continue; 
		if ( ay >= MAX(p1.y, p2.y)) 
			continue; 
		double x = (double)(ay - p1.y) * (double)(p2.x - p1.x) / (double)(p2.y - p1.y) + p1.x; 
		if ( x > ax ) 
			nCross++;
//		printf("i=%d,ax=%d ay=%d  p1 %d %d p2 %d %d cross=%d\n",i, ax,ay,p1.x,p1.y,p2.x,p2.y,nCross);
	}
	if (nCross % 2 ==1)	//单边交点为偶数，点在多边形之外 
	{
		return TRUE;
	}
	return FALSE;

}



//根据客户端DLL画线的配置设置检测方向?
//上下左右的设置需要对应的邋A->B B-> A<->B
static int  mmva_get_tripline_checkdirect(int x0,int y0, int x1,int y1, int nIvpCheckMode)
{
	int deltax,deltay;
	deltax = x1 - x0;
	deltay = y1 - y0;
	
		if(nIvpCheckMode == 0)							//A->B检测单向
		{
			if(abs(deltax) > abs(deltay))
			{
				if(deltax	> 0)
				{
					if(deltay >=0)
					{

						return e_IVP_CHECK_MODE_LINE_D2U;
					}
					else
					{
						return e_IVP_CHECK_MODE_LINE_U2D;

					}

				}
				else
				{

					if(deltay >=0)
					{

						return e_IVP_CHECK_MODE_LINE_U2D;
					}
					else
					{
						return e_IVP_CHECK_MODE_LINE_D2U;

					}


				}
			}
			else
			{

				return e_IVP_CHECK_MODE_LINE_L2R;
			}

		}
		else if(nIvpCheckMode == 1)						//B->A检测
		{
			if(abs(deltax) > abs(deltay))
			{
				if(deltax	> 0)
				{
					if(deltay >=0)
					{

						return e_IVP_CHECK_MODE_LINE_U2D;
					}
					else
					{
						return e_IVP_CHECK_MODE_LINE_D2U;

					}

				}
				else
				{

					if(deltay >=0)
					{

						return e_IVP_CHECK_MODE_LINE_D2U;
					}
					else
					{
						return e_IVP_CHECK_MODE_LINE_U2D;

					}


				}
			}
			else
			{

				return e_IVP_CHECK_MODE_LINE_R2L;
			}

		}
		return e_IVP_CHECK_MODE_LINE_LR;
}
//绊线检测，按照方向A->B B-> A<->B判定

 static void  mmva_check_tripline(MIVPREGION_t 	*pStRegion, ObjStaticsInfo_t *ObjStatics, int objNum)
{
	int i,j,k;
	BOOL bIntersect;
	BOOL bRegionTrigger;
	int x,y,x_begin,y_begin;
	IVP_CHECK_MODE checkmode;
	int max_x,min_x,max_y,min_y;
	int vector_x ,vector_y; 
	if(pStRegion->nCnt!=2)
		return;
	bRegionTrigger  = FALSE;
	max_y =	MAX(pStRegion->stPoints[0].y,pStRegion->stPoints[1].y)*ATK_VI_H/stMmva.viH;
	min_y =	MIN(pStRegion->stPoints[0].y,pStRegion->stPoints[1].y)*ATK_VI_H/stMmva.viH;
	max_x =	MAX(pStRegion->stPoints[0].x,pStRegion->stPoints[1].x)*ATK_VI_W/stMmva.viW;
	min_x =	MIN(pStRegion->stPoints[0].x,pStRegion->stPoints[1].x)*ATK_VI_W/stMmva.viW;
	for(i=0;i< objNum;i++)
	{
		if(ObjStatics[i].repeats < 10)
			continue;
		bIntersect = mmva_judge_rectangle_and_line_intersect(pStRegion->stPoints[0].x,pStRegion->stPoints[0].y,pStRegion->stPoints[1].x,pStRegion->stPoints[1].y,\
															ObjStatics[i].left, ObjStatics[i].top, ObjStatics[i].right, ObjStatics[i].bottom,ATK_VI_W,stMmva.viW,ATK_VI_H,stMmva.viH);
		if(bIntersect)
		{
			x = (ObjStatics[i].left + ObjStatics[i].right)>>1;
			y = (ObjStatics[i].top + ObjStatics[i].bottom)>>1;

			vector_x = ObjStatics[i].x_buf[0] - ObjStatics[i].x_buf[9];
			vector_y = ObjStatics[i].y_buf[0] - ObjStatics[i].y_buf[9];
//			checkmode = mmva_get_tripline_checkdirect(pStRegion);
			checkmode = mmva_get_tripline_checkdirect(pStRegion->stPoints[0].x,pStRegion->stPoints[0].y,pStRegion->stPoints[1].x,pStRegion->stPoints[1].y,pStRegion->nIvpCheckMode);

			if(checkmode == e_IVP_CHECK_MODE_LINE_L2R)
			{
				if((vector_x > MOTION_VECTOR_THRESHHOLD)&& (ObjStatics[i].right > min_x) )
				{
					ObjStatics[i].bAlarmed = TRUE;
					bRegionTrigger  = TRUE;
					if(pStRegion->staytimeout <= 0)
					{
						pStRegion->eAlarmType = e_IPV_ALARM_TYPE_L2R   ;
					}
					pStRegion->staytimeout = stMmva.nAlarmtime;
					printf("checkmode=%d ,e_IPV_ALARM_TYPE_L2R\n",checkmode);
				}
			}
			else if(checkmode == e_IVP_CHECK_MODE_LINE_R2L)
			{
				if((vector_x < -MOTION_VECTOR_THRESHHOLD)&& (ObjStatics[i].left < max_x) )
				{
					ObjStatics[i].bAlarmed = TRUE;
					bRegionTrigger  = TRUE;
					if(pStRegion->staytimeout <= 0)
					{
						pStRegion->eAlarmType = e_IPV_ALARM_TYPE_R2L   ;
					}
					pStRegion->staytimeout = stMmva.nAlarmtime;	
					printf("checkmode=%d,e_IPV_ALARM_TYPE_R2L\n",checkmode);
				}

			}
			else if(checkmode == e_IVP_CHECK_MODE_LINE_LR)
			{
				ObjStatics[i].bAlarmed = TRUE;
				bRegionTrigger  = TRUE;
				if(pStRegion->staytimeout <= 0)
				{
					pStRegion->eAlarmType = e_IPV_ALARM_TYPE_LR   ;
				}
				pStRegion->staytimeout = stMmva.nAlarmtime;						

			}
			else if(checkmode == e_IVP_CHECK_MODE_LINE_U2D)
			{

				if((vector_y > MOTION_VECTOR_THRESHHOLD) && (ObjStatics[i].bottom> min_y))
				{
					ObjStatics[i].bAlarmed = TRUE;
					bRegionTrigger  = TRUE;
					if(pStRegion->staytimeout <= 0)
					{
						pStRegion->eAlarmType = e_IPV_ALARM_TYPE_U2D   ;
					}
					pStRegion->staytimeout = stMmva.nAlarmtime;		
					printf("checkmod=%d,ee_IPV_ALARM_TYPE_U2D\n",checkmode);
				}
					//printf("i=%d  vectory y=%d  y0=%d  y9=%d  min_y=%d maxy= %d\n",i,vector_y , ObjStatics[i].y_buf[0], ObjStatics[i].y_buf[9], min_y,max_y);

			}
			else if(checkmode == e_IVP_CHECK_MODE_LINE_D2U)
			{
				if((vector_y < -MOTION_VECTOR_THRESHHOLD)&&(ObjStatics[i].top < max_y) )
				{
					ObjStatics[i].bAlarmed = TRUE;
					bRegionTrigger  = TRUE;
					
					if(pStRegion->staytimeout <= 0)
					{
						pStRegion->eAlarmType = e_IPV_ALARM_TYPE_D2U   ;
					}
					pStRegion->staytimeout = stMmva.nAlarmtime;	
					printf("checkmode=%d,e_IPV_ALARM_TYPE_D2U\n",checkmode);
					//printf("i=%d  vectory y=%d  y=%d  min_y=%d maxy= %d\n",i,vector_y ,y, min_y,max_y);

				}

			}
			if((pStRegion->staytimeout > 0) && bRegionTrigger)
				pStRegion->flickertimeout = ALARM_OSDFLICKER_KEEP_TIMEOUTS;
//			printf("i= %d, Trip-line Alarm %d %d %d %d\n ", ObjStatics[i].left,ObjStatics[i].top,ObjStatics[i].right, ObjStatics[i].bottom );

		}
	}

}

//绊线检测，按照方向A->B B-> A<->B判定

 static void  mmva_check_climb(MIVP_RL_t *pRule, ObjStaticsInfo_t *ObjStatics, int objNum)
{
	int i,j,k;
	BOOL bIntersect;
	BOOL bRegionTrigger;
	int x,y,x_begin,y_begin;
	IVP_CHECK_MODE checkmode;
	int max_x,min_x,max_y,min_y;
	int vector_x ,vector_y; 
	int height ,thresheight;
	if(pRule->stClimb.nPoints != 2)
		return;
	bRegionTrigger  = FALSE;
	max_y =	MAX(pRule->stClimb.stPoints[0].y,pRule->stClimb.stPoints[1].y)*ATK_VI_H/stMmva.viH;
	min_y =	MIN(pRule->stClimb.stPoints[0].y,pRule->stClimb.stPoints[1].y)*ATK_VI_H/stMmva.viH;
	max_x =	MAX(pRule->stClimb.stPoints[0].x,pRule->stClimb.stPoints[1].x)*ATK_VI_W/stMmva.viW;
	min_x =	MIN(pRule->stClimb.stPoints[0].x,pRule->stClimb.stPoints[1].x)*ATK_VI_W/stMmva.viW;
	thresheight = ATK_VI_H - MIN(pRule->stClimb.stPoints[0].y, pRule->stClimb.stPoints[1].y)*ATK_VI_H/stMmva.viH;;
	for(i=0;i< objNum;i++)
	{
		bIntersect = mmva_judge_rectangle_and_line_intersect(pRule->stClimb.stPoints[0].x,pRule->stClimb.stPoints[0].y,pRule->stClimb.stPoints[1].x,pRule->stClimb.stPoints[1].y,\
															ObjStatics[i].left, ObjStatics[i].top, ObjStatics[i].right, ObjStatics[i].bottom,ATK_VI_W,stMmva.viW,ATK_VI_H,stMmva.viH);
		if(bIntersect)
		{
			height = abs(ObjStatics[i].top - ObjStatics[i].bottom);
			if((height <  thresheight*1.1) &&(height > thresheight*0.4) && (ObjStatics[i].top < max_y))
			{
				ObjStatics[i].IntersectCountsClimb++;

			}
			else
			{
				ObjStatics[i].IntersectCountsClimb=0;
			}
			//printf("i=%d ,counts=%d height=%d thresh=%d \n ",i,ObjStatics[i].IntersectCountsClimb,height,thresheight);

//			else
//				ObjStatics[i].IntersectCountsClimb = 0
			if(ObjStatics[i].IntersectCountsClimb < 5)
				continue;
			if(ObjStatics[i].repeats < 9)
				continue;
			ObjStatics[i].IntersectCountsClimb = 0;
//			x = (ObjStatics[i].left + ObjStatics[i].right)>>1;
			y = (ObjStatics[i].top + ObjStatics[i].bottom)>>1;
			
			vector_y = ObjStatics[i].top - ObjStatics[i].top_buf[9];

			{
				if((vector_y < -5)&&(ObjStatics[i].top < max_y) )
				{
					ObjStatics[i].bAlarmed = TRUE;
					bRegionTrigger  = TRUE;
					
					if(pRule->stClimb.staytimeout <= 0)
					{
						pRule->stClimb.eAlarmType = e_IPV_ALARM_TYPE_D2U   ;
					}
					pRule->stClimb.staytimeout = stMmva.nAlarmtime;	
					//printf("i=%d  vectory y=%d  y=%d  min_y=%d maxy= %d\n",i,vector_y ,y, min_y,max_y);

				}

			}			
			if((pRule->stClimb.staytimeout > 0) && bRegionTrigger)
				pRule->stClimb.flickertimeout = ALARM_OSDFLICKER_KEEP_TIMEOUTS;
//			printf("i= %d, Trip-line Alarm %d %d %d %d\n ", ObjStatics[i].left,ObjStatics[i].top,ObjStatics[i].right, ObjStatics[i].bottom );

		}
	}

}

//区域入侵 :判断进入或者离开区域

 static void  mmva_check_region_inout(MIVPREGION_t 	*pStRegion, ObjStaticsInfo_t *ObjStatics, int objNum)
{
	int i,j,next;
	BOOL bIntersect;
	BOOL bRegionTrigger;
	int polygon_x, polygon_y;
	int dis1,dis2;
	int x,y,x_begin,y_begin;
	if(pStRegion->nCnt < 3)
		return;
	bIntersect = FALSE;
	bRegionTrigger  = FALSE;
//	printf("objNum=%d\n ",objNum);
	for(i=0;i< objNum;i++)
	{
		
		//矩形和多边形相交的情况
		for(j=0; j< pStRegion->nCnt;j++)
		{
			if(ObjStatics[i].repeats < 9 )
				continue;
			next = (j+1)%(pStRegion->nCnt);
			bIntersect = mmva_judge_rectangle_and_line_intersect(pStRegion->stPoints[j].x,pStRegion->stPoints[j].y,pStRegion->stPoints[next].x,pStRegion->stPoints[next].y, \
									ObjStatics[i].left, ObjStatics[i].top, ObjStatics[i].right, ObjStatics[i].bottom, ATK_VI_W, stMmva.viW, ATK_VI_H,stMmva.viH);
			if(bIntersect)
			{
				break;
			}
		}
	#if 0
		//矩形在多边形内部的情况，暂时不认为是触发报警的
		if(bIntersect == FALSE)
		{
				if(mmva_judge_point_in_region(ObjStatics[i].left,ObjStatics[i].top, pStRegion,ATK_VI_W,stMmva.viW,ATK_VI_H,stMmva.viH))
				{
					bIntersect = TRUE;
				}
				else if(mmva_judge_point_in_region(ObjStatics[i].left,ObjStatics[i].bottom,pStRegion,ATK_VI_W,stMmva.viW,ATK_VI_H,stMmva.viH))
				{
					bIntersect = TRUE;
				}
				else if(mmva_judge_point_in_region(ObjStatics[i].right,ObjStatics[i].bottom,pStRegion,ATK_VI_W,stMmva.viW,ATK_VI_H,stMmva.viH))
				{
					bIntersect = TRUE;
				}
				else if(mmva_judge_point_in_region(ObjStatics[i].right,ObjStatics[i].top,pStRegion,ATK_VI_W,stMmva.viW,ATK_VI_H,stMmva.viH))
				{
					bIntersect = TRUE;
				}
		}
	#endif
		if(bIntersect)
		{
			x = (ObjStatics[i].left + ObjStatics[i].right)>>1;
			y = (ObjStatics[i].top + ObjStatics[i].bottom)>>1;
//			x_begin= x - ObjStatics[i].vector_x;
//			y_begin= y - ObjStatics[i].vector_y;
			x_begin= ObjStatics[i].x_buf[7];
			y_begin= ObjStatics[i].y_buf[7];
			ObjStatics[i].bAlarmed = TRUE;
			polygon_x =0 ;
			polygon_y =0 ;
			for(j=0; j< pStRegion->nCnt;j++)
			{
				polygon_x += pStRegion->stPoints[j].x;
				polygon_y += pStRegion->stPoints[j].y;
			}
			polygon_x += polygon_x*ATK_VI_W/(pStRegion->nCnt * stMmva.viW);
			polygon_y += polygon_y*ATK_VI_H/(pStRegion->nCnt * stMmva.viH);

			dis1 = abs(x - polygon_x + y  - polygon_y);
			dis2 = abs(x_begin- polygon_x + y_begin- polygon_y);
			if(pStRegion->staytimeout <= 0)
			{
				pStRegion->staytimeout = stMmva.nAlarmtime;
				if(dis1 > dis2)		//离开区域
				{
					pStRegion->eAlarmType = e_IPV_ALARM_TYPE_OUT;
				}
				else				//进入区域
				{
					pStRegion->eAlarmType = e_IPV_ALARM_TYPE_IN ;
				}
			}
			else
			{
				pStRegion->staytimeout = stMmva.nAlarmtime;
				pStRegion->flickertimeout = ALARM_OSDFLICKER_KEEP_TIMEOUTS;
			}
			bRegionTrigger  = TRUE;


		}

		
	}

}

//区域入侵和绊线检测
 static void  mmva_check_invasion( MIVP_RL_t *pRule, ObjStaticsInfo_t *ObjStatics, int objNum)
{
	int i;
	if(pRule->bEnable==FALSE)
		return;
	if(pRule->nRgnCnt < 1)
		return;
	
	for(i=0;i<pRule->nRgnCnt;i++)
	{
		if(pRule->stRegion[i].nCnt == 2)	//绊线检测
		{
			mmva_check_tripline(&pRule->stRegion[i], ObjStatics,  objNum);

		}
		else  if(pRule->stRegion[i].nCnt > 2)	//绊线检测
		{
			mmva_check_region_inout(&pRule->stRegion[i], ObjStatics,  objNum);

		}


		

	}

}
//获取空闲的index
static int mmva_get_freeIndex(ObjStaticsInfo_t *pStObj ,int num)
{
	int i;
	for(i=0;i<num;i++)
	{
		if(pStObj[i].staytimeouts < 1)
			return i;
	}

	return -1;
}


//过线计数的统计
//垂直非连续、非对向行走精度达到95%以上，20°倾角90%以上
 static void  __mmva_linecount( MIVP_Count_t *pRule, ObjStaticsInfo_t *ObjStatics, int objNum)
{
	int i;
	int x,x_begin, y,y_begin;
	int a,b;
	int max_y,max_x,min_y,min_x;
	int diff;
	int checkmode;
	POINT_S point[2];
	BOOL bintersect;
	if(pRule->bOpenCount !=TRUE)
		return;
	if(pRule->nPoints  != 2)
		return;
	for(i=0; i<2; i++)
	{
		point[i].s32X = pRule->stPoints[i].x * ATK_VI_W/stMmva.viW;
		point[i].s32Y = pRule->stPoints[i].y * ATK_VI_H/stMmva.viH;
	}

	checkmode = mmva_get_tripline_checkdirect(pRule->stPoints[0].x,pRule->stPoints[0].y,pRule->stPoints[1].x,pRule->stPoints[1].y,pRule->nCheckMode);
	
	for(i=0;i< objNum;i++)
	{
		if((ObjStatics[i].w > ATK_VI_W*0.8) || (ObjStatics[i].w <  ATK_VI_W/10))
		{
//			printf(" too big or small , i= %d w= %d\n",i,stMmva.ObjStatics[i].w);
			continue;
		}
		if(ObjStatics[i].bCounted)
			continue;
//		if(ObjStatics[i].repeats > MINI_REPEAT_NUM)	//要求连续出现8次
		{
			y = (ObjStatics[i].bottom + ObjStatics[i].top)/2;
			y_begin = (ObjStatics[i].bottom_begin + ObjStatics[i].top_begin)/2;
		 	x = (ObjStatics[i].right + ObjStatics[i].left)/2;
		 	x_begin = (ObjStatics[i].right_begin + ObjStatics[i].left_begin)/2;
			bintersect =  mmva_judge_line_intersect(point[0].s32X,point[0].s32Y,point[1].s32X,point[1].s32Y, x,y,x_begin,y_begin);
			 if(bintersect)	//中心点相交
			 {

				ObjStatics[i].IntersectCounts++;
				
//				printf("i=%d ,IntersectCounts=%d \n",i,ObjStatics[i].IntersectCounts);

			 }
		 	if(ObjStatics[i].IntersectCounts > 3)
			{
//				printf("check mode =%d\n",checkmode);


//				max_y = MAX(point[0].s32Y,point[1].s32Y);
//				min_y = MIN(point[0].s32Y,point[1].s32Y);
//				max_x = MAX(point[0].s32X,point[1].s32X);
//				min_x = MIN(point[0].s32X,point[1].s32X);
				max_y = (point[0].s32Y+point[1].s32Y)>>1;
				min_y = (point[0].s32Y+point[1].s32Y)>>1;
				max_x = (point[0].s32X+point[1].s32X)>>1;
				min_x = (point[0].s32X+point[1].s32X)>>1;

				if(checkmode == e_IVP_CHECK_MODE_LINE_U2D)
				{
					if(y > y_begin)
					{
						diff =  ObjStatics[i].top - max_y;
						if(diff > 0 )	//分离达到一定门限再加1
						{
							stMmva.count_in++;
							ObjStatics[i].bCounted =TRUE;
							ObjStatics[i].IntersectCounts = 0;
						}
					}
					else if(y < y_begin)
					{
						diff = min_y - ObjStatics[i].bottom;
						if(diff > 0 )	//分离达到一定门限再加1
						{
							stMmva.count_out++;
							ObjStatics[i].bCounted =TRUE;
							ObjStatics[i].IntersectCounts = 0;
						}

					}
//					printf("diff=%d,y=%d,ybegin=%d\n"  ,diff,y,y_begin);
						
				}
				else if(checkmode == e_IVP_CHECK_MODE_LINE_D2U)
				{

					if(y > y_begin)
					{
						diff =  ObjStatics[i].top - max_y;
						if(diff > 0 )	//分离达到一定门限再加1
						{
							stMmva.count_out++;
							ObjStatics[i].bCounted =TRUE;
							ObjStatics[i].IntersectCounts = 0;

						}
					}
					else if(y < y_begin)
					{
						diff = min_y - ObjStatics[i].bottom;
						if(diff > 0 )	//分离达到一定门限再加1
						{
							ObjStatics[i].bCounted =TRUE;
							ObjStatics[i].IntersectCounts = 0;
							stMmva.count_in++;
						}
					}
//					printf("diff=%d,y=%d,ybegin=%d\n"  ,diff,y,y_begin);

				}
				else if(checkmode == e_IVP_CHECK_MODE_LINE_L2R)
				{

					if(x > x_begin)
					{
						diff =  ObjStatics[i].left - max_x;
						if(diff > 0 )	//分离达到一定门限再加1
						{
							stMmva.count_in++;
							ObjStatics[i].bCounted =TRUE;
							ObjStatics[i].IntersectCounts = 0;
						}
					}
					else if(x < x_begin)
					{
						diff = min_x - ObjStatics[i].right;
						if(diff > 0 )	//分离达到一定门限再加1
						{
							stMmva.count_out++;
							ObjStatics[i].bCounted =TRUE;
							ObjStatics[i].IntersectCounts = 0;
						}

					}
//					printf("diff=%d,x=%d,xbegin=%d\n"  ,diff,x,x_begin);



				}
				else if(checkmode == e_IVP_CHECK_MODE_LINE_R2L)
				{
					if(x > x_begin)
					{
						diff =  ObjStatics[i].left - max_x;
						if(diff > 0 )	//分离达到一定门限再加1
						{
							stMmva.count_out++;
							ObjStatics[i].bCounted =TRUE;
							ObjStatics[i].IntersectCounts = 0;
						}
					}
					else if(x < x_begin)
					{
						diff = min_x - ObjStatics[i].right;
						if(diff > 0 )	//分离达到一定门限再加1
						{
							stMmva.count_in++;
							ObjStatics[i].bCounted =TRUE;
							ObjStatics[i].IntersectCounts = 0;
						}

					}
//					printf("diff=%d,x=%d,xbegin=%d\n"  ,diff,x,x_begin);

				}
				
				
			}
			
		}



	}



}

//清除报警计数器
static BOOL __mmva_timer_callback(int tid, void *param)
{
	int i;
	if( _mmva_alram!=NULL)
		_mmva_alram(NULL, 0, e_IPV_ALARM_TYPE_CLEAR, NULL);

	stMmva.bHideAlarmStart = FALSE;	
	
	for(i=0; i<stMivp.st_rl_attr.nRgnCnt; i++)
	{
		stMivp.st_rl_attr.stRegion[i].bAlarmed =FALSE;
		stMivp.st_rl_attr.stRegion[i].staytimeout = 0;
		stMivp.st_rl_attr.stRegion[i].flickertimeout = 0;
		stMivp.st_rl_attr.stRegion[i].eAlarmType = e_IPV_ALARM_TYPE_CLEAR;


	}
	stMivp.st_rl_attr.stClimb.bAlarmed = FALSE;
	stMivp.st_rl_attr.stClimb.staytimeout = 0;
	stMivp.st_rl_attr.stClimb.flickertimeout = 0;
	stMivp.st_rl_attr.stClimb.eAlarmType = e_IPV_ALARM_TYPE_CLEAR;
	return 0;
}

//区域入侵绊线报警上报
static void __mmva_region_line_alarm_report(MIVP_RL_t *pRule )
{
	int i;
	static U64 lastime = 0;
	U64 now;
	BOOL bIgnore;
	now = utl_get_ms()/1000;
	if(now > lastime )
	{
		bIgnore = 0;
		lastime=now;

	}
	else
	{
		bIgnore = 1;

	}
	ALARMSET stAlarm;
	malarm_get_param(&stAlarm);
	stMmva.nAlarmtime = stAlarm.delay;
	for(i=0; i<pRule->nRgnCnt; i++ )
	{

		if(pRule->stRegion[i].staytimeout > 0)
		{
			if(pRule->stRegion[i].eAlarmType > 0)
			{
				if(stMmva.mva_tid > 0)
					utl_timer_reset(stMmva.mva_tid, stMmva.nAlarmtime*1000, __mmva_timer_callback, &i);
				if(pRule->stRegion[i].bAlarmed ==FALSE)
				{
					pRule->stRegion[i].bAlarmed = TRUE;
					_mmva_alram(NULL, i, pRule->stRegion[i].eAlarmType, NULL);	//每次都发报警信号
				}
					
			}
		}
		if(!bIgnore)
		{
			if(pRule->stRegion[i].flickertimeout > 0)
				pRule->stRegion[i].flickertimeout--; 
			if(pRule->stRegion[i].staytimeout > 0)
			{
				pRule->stRegion[i].staytimeout --;
			}				
		}


	}
	if(pRule->stClimb.nPoints ==2 )
	{
		if(pRule->stClimb.staytimeout > 0)
		{
			if(pRule->stClimb.eAlarmType > 0)
			{
				if(stMmva.mva_tid > 0)
					utl_timer_reset(stMmva.mva_tid, stMmva.nAlarmtime*1000, __mmva_timer_callback, &i);
				if(pRule->stClimb.bAlarmed ==FALSE)
				{
					pRule->stClimb.bAlarmed = TRUE;
					_mmva_alram(NULL, i, pRule->stClimb.eAlarmType, NULL);	//每次都发报警信号
					
				}

			}

		}
		if(!bIgnore)
		{
			if(pRule->stClimb.flickertimeout > 0)
				pRule->stClimb.flickertimeout--; 
			if(pRule->stClimb.staytimeout > 0)
			{
				pRule->stClimb.staytimeout --;
			}				
		}



	}


}


//MVA运动分析，由jv_mdetect线程调用
//包含区域入侵、绊线、过线计数
 int mmva_ccl_analysis( ObjStaticsInfo_t *stObj, int * ObjNum)
{
	int i,j,k;
	ObjStaticsInfo_t ObjStaticsTmp[IVE_MAX_REGION_NUM];
	int Num,index;
	int seed;
	BOOL bMerged;
	BOOL bTheSameLable;
	BOOL bTheFirstSame;
	if(!bInited)
		return 0;		
	if(delaywork < DELAYWORK_TIMEOUT)	//开机后200帧以内不工作
		return 0;		
	if((!stMivp.st_rl_attr.bEnable) && (!stMivp.st_count_attr.bOpenCount))
		return 0;
	Num= *ObjNum;

	for(i=0;i<Num;i++)
	{
		bTheSameLable=0;
		for(j=0;j<stMmva.objNum;j++)
		{
			//计算距离，太小，则认为是同一个物体
			if(mmva_judge_theSameObjs(&stMmva.ObjStatics[j], &stObj[i],OBJ_MOTION_THESAME_LIMIT))
			{	
				bTheSameLable =1 ;
				
				break;
			}

		}
		memcpy(&ObjStaticsTmp[i],&stObj[i],sizeof(ObjStaticsInfo_t));
		if(bTheSameLable)
		{
			ObjStaticsTmp[i].lable = stMmva.ObjStatics[j].lable;
			ObjStaticsTmp[i].repeats =  stMmva.ObjStatics[j].repeats +1;
			ObjStaticsTmp[i].bCounted= stMmva.ObjStatics[j].bCounted;
			ObjStaticsTmp[i].IntersectCounts = stMmva.ObjStatics[j].IntersectCounts;
			ObjStaticsTmp[i].IntersectCountsClimb = stMmva.ObjStatics[j].IntersectCountsClimb;

			mmva_copy_beginpos(&ObjStaticsTmp[i],&stMmva.ObjStatics[j]);
			ObjStaticsTmp[i].vector_x = (ObjStaticsTmp[i].right +  ObjStaticsTmp[i].left - stMmva.ObjStatics[j].right - stMmva.ObjStatics[j].left)/2;
			ObjStaticsTmp[i].vector_y = (ObjStaticsTmp[i].bottom +  ObjStaticsTmp[i].top - stMmva.ObjStatics[j].bottom - stMmva.ObjStatics[j].top)/2;
			memcpy(&ObjStaticsTmp[i].x_buf ,&stMmva.ObjStatics[j].x_buf,sizeof(int)*10);
			memcpy(&ObjStaticsTmp[i].y_buf ,&stMmva.ObjStatics[j].y_buf,sizeof(int)*10);
			memcpy(&ObjStaticsTmp[i].top_buf ,&stMmva.ObjStatics[j].top_buf,sizeof(int)*10);
			memcpy(&ObjStaticsTmp[i].bottom_buf ,&stMmva.ObjStatics[j].bottom_buf,sizeof(int)*10);
			for(k=9;k>0;k--)
			{
				ObjStaticsTmp[i].x_buf[k] = ObjStaticsTmp[i].x_buf[k-1]; 
				ObjStaticsTmp[i].y_buf[k] = ObjStaticsTmp[i].y_buf[k-1]; 
				ObjStaticsTmp[i].top_buf[k] = ObjStaticsTmp[i].top_buf[k-1]; 
				ObjStaticsTmp[i].bottom_buf[k] = ObjStaticsTmp[i].bottom_buf[k-1]; 				
				
			}
			ObjStaticsTmp[i].x_buf[0] = (stMmva.ObjStatics[j].right + stMmva.ObjStatics[j].left)/2;
			ObjStaticsTmp[i].y_buf[0] = ( stMmva.ObjStatics[j].bottom + stMmva.ObjStatics[j].top)/2;
			ObjStaticsTmp[i].top_buf[0] = stMmva.ObjStatics[j].top;
			ObjStaticsTmp[i].bottom_buf[0] = stMmva.ObjStatics[j].bottom;

		}
		else
		{
			ObjStaticsTmp[i].repeats = 0;
			ObjStaticsTmp[i].bCounted= FALSE;
			ObjStaticsTmp[i].IntersectCounts = 0;
			ObjStaticsTmp[i].IntersectCountsClimb = 0;
			mmva_copy_pos2beginpos(&ObjStaticsTmp[i],&ObjStaticsTmp[i]);
			ObjStaticsTmp[i].vector_x = 0;
			ObjStaticsTmp[i].vector_y = 0;
			
		}


	}

#if 1
//同一起始位置的物体边界要扩充
	for(i=0;i<Num;i++)
 	{
			
		for(j=0;j<Num;j++)
		{
			if((ObjStaticsTmp[j].left==ObjStaticsTmp[i].left) && (ObjStaticsTmp[j].right==ObjStaticsTmp[i].right) && (ObjStaticsTmp[j].bottom==ObjStaticsTmp[i].bottom)&&(ObjStaticsTmp[j].top==ObjStaticsTmp[i].top))
			{
				continue;	//防止死循环
			}
			else if((ObjStaticsTmp[i].left_begin==ObjStaticsTmp[j].left_begin ) && ( ObjStaticsTmp[i].right_begin==ObjStaticsTmp[j].right_begin) && (ObjStaticsTmp[i].top_begin==ObjStaticsTmp[j].top_begin)  &&  (ObjStaticsTmp[i].bottom_begin==ObjStaticsTmp[j].bottom_begin) )
			{
				ccl_merge(&ObjStaticsTmp[i],&ObjStaticsTmp[j]);
				i=0;
				j=0;
			}



		}

	}
#endif
#if 0
//第二次合并相邻的连通域
		 for(i = 0; i < Num; i++)
		 {
		 	for(j=0;j<Num;j++)
		 	{
				if((ObjStaticsTmp[j].left==ObjStaticsTmp[i].left) && (ObjStaticsTmp[j].right==ObjStaticsTmp[i].right) && (ObjStaticsTmp[j].bottom==ObjStaticsTmp[i].bottom)&&(ObjStaticsTmp[j].top==ObjStaticsTmp[i].top))
				{
					continue;	//防止死循环
				}
				else if(__bNearObjs(&ObjStaticsTmp[j],&ObjStaticsTmp[i],OBJ_TIMEREGION_MERGE_LIMIT))
					
				{
					ccl_merge(&ObjStaticsTmp[j],&ObjStaticsTmp[i]);
					i=0;
					j=0;

				}
		 	}

//			printf(" merge :l %d  r %d  t %d  b %d ;origin:l %d  r %d  t %d b %d  \n" ,atkinfo.ObjStatics[j].left,atkinfo.ObjStatics[j].right,atkinfo.ObjStatics[j].top,atkinfo.ObjStatics[j].bottom, ObjStatics[j].left,ObjStatics[j].right,ObjStatics[j].top,ObjStatics[j].bottom);
		 }
#endif
	//去掉重复的物体
	for(i=0;i<Num;i++)
 	{
			
		for(j=i+1;j<Num;j++)
		{
			
			if((ObjStaticsTmp[i].left==ObjStaticsTmp[j].left ) && ( ObjStaticsTmp[i].right==ObjStaticsTmp[j].right) && (ObjStaticsTmp[i].top==ObjStaticsTmp[j].top)  &&  (ObjStaticsTmp[i].bottom==ObjStaticsTmp[j].bottom) )
			{
				for(k=j;k<(Num-1);k++)
				{

					memcpy(&ObjStaticsTmp[k],&ObjStaticsTmp[k+1],sizeof(ObjStaticsInfo_t) );

				}
				--Num;
				--j;
			}



		}


	}
	
	for(i=0;i<Num;i++)
	{
			ObjStaticsTmp[i].w = (ObjStaticsTmp[i].w >>1)<<1;
			ObjStaticsTmp[i].h = (ObjStaticsTmp[i].h >>1)<<1;
			memcpy(&stMmva.ObjStatics[i],&ObjStaticsTmp[i],sizeof(ObjStaticsInfo_t));
	}

	 stMmva.objNum =  Num;


	if(stMivp.st_rl_attr.bEnable )	//绊线检测和区域入侵检测
	{
		mmva_check_invasion(&stMivp.st_rl_attr,&stMmva.ObjStatics[0],stMmva.objNum);
		mmva_check_climb(&stMivp.st_rl_attr, &stMmva.ObjStatics[0],  stMmva.objNum);
		__mmva_region_line_alarm_report(&stMivp.st_rl_attr );

	}


	if(stMmva.bLinecountSupport)
		__mmva_linecount(&stMivp.st_count_attr,&stMmva.ObjStatics[0],stMmva.objNum);

	for(i=0;i<Num;i++)
	{
		bMerged = FALSE;
		for(j=0;j<IVE_MAX_REGION_NUM;j++)
		{
			if(stMmva.ObjFilterStatics[j].staytimeouts > 0)
			{
				if(mmva_judge_theSameObjs(&stMmva.ObjStatics[i], &stMmva.ObjFilterStatics[j],OBJ_MOTION_THESAME_LIMIT))	//和历史绘图是同一个物体那么更新坐标
				{
					if(bMerged == FALSE)
					{
						int bCounted = stMmva.ObjFilterStatics[j].bCounted;
						int IntersectCounts = stMmva.ObjFilterStatics[j].IntersectCounts;
						int IntersectCountsClimb = stMmva.ObjFilterStatics[j].IntersectCountsClimb;
						BOOL bHisAlarmed = stMmva.ObjFilterStatics[j].bAlarmed;
						memcpy(&stMmva.ObjFilterStatics[j],&stMmva.ObjStatics[i],sizeof(ObjStaticsInfo_t));		//更新旧物体的位置
						stMmva.ObjFilterStatics[j].bCounted = bCounted;
						stMmva.ObjFilterStatics[j].IntersectCounts =IntersectCounts;
						stMmva.ObjFilterStatics[j].IntersectCountsClimb =IntersectCountsClimb;
						stMmva.ObjFilterStatics[j].staytimeouts = OSD_STAY_TIMEOUTS;
						stMmva.ObjFilterStatics[j].lasttime = utl_get_ms();
						if(bHisAlarmed || stMmva.ObjStatics[i].bAlarmed)
							stMmva.ObjFilterStatics[j].bAlarmed =TRUE;
						bMerged = TRUE;
					}
					else
					{
						stMmva.ObjFilterStatics[j].staytimeouts = 0;
				
					}
					
 
				}
			}
		}
		if((bMerged ==FALSE)  && (stMmva.ObjStatics[i].repeats >1))
		{
			index=mmva_get_freeIndex(&stMmva.ObjFilterStatics[0],IVE_MAX_REGION_NUM);
			if(index<0)
			{
				index = 0;
				printf("Error : %s :%d get free ObjStaticsInfo_t failed\n",__func__,__LINE__);
			}

			memcpy(&stMmva.ObjFilterStatics[index],&stMmva.ObjStatics[i],sizeof(ObjStaticsInfo_t));		//加入新物体
			stMmva.ObjFilterStatics[index].staytimeouts = OSD_STAY_TIMEOUTS;
			stMmva.ObjFilterStatics[index].lasttime = utl_get_ms();


		}


	}

	U64 now = utl_get_ms();
	for(i=0;i<IVE_MAX_REGION_NUM;i++)
	{
		if(stMmva.ObjFilterStatics[i].staytimeouts >0)
		{
			if(now - stMmva.ObjFilterStatics[i].lasttime > 50)
			{
				 stMmva.ObjFilterStatics[i].lasttime = now;
				 stMmva.ObjFilterStatics[i].staytimeouts --;

					if(stMmva.ObjFilterStatics[i].w > OSD_DISP_MIN_SIZE)
					{
						stMmva.ObjFilterStatics[i].left +=  OSD_DISP_STEP_LENGTH;
						stMmva.ObjFilterStatics[i].right  -= OSD_DISP_STEP_LENGTH;
						stMmva.ObjFilterStatics[i].w = stMmva.ObjFilterStatics[i].right - stMmva.ObjFilterStatics[i].left+1;
					}
					if(stMmva.ObjFilterStatics[i].h > OSD_DISP_MIN_SIZE)
					{
						stMmva.ObjFilterStatics[i].top += OSD_DISP_STEP_LENGTH;
						stMmva.ObjFilterStatics[i].bottom  -= OSD_DISP_STEP_LENGTH;
						stMmva.ObjFilterStatics[i].h = stMmva.ObjFilterStatics[i].bottom - stMmva.ObjFilterStatics[i].top+1;
					}
					if((stMmva.ObjFilterStatics[i].w <= OSD_DISP_MIN_SIZE) || (stMmva.ObjFilterStatics[i].h <= OSD_DISP_MIN_SIZE))
					{
						stMmva.ObjFilterStatics[i].staytimeouts = 0;
						
					}
					if((stMmva.ObjFilterStatics[i].w <= 0) || (stMmva.ObjFilterStatics[i].h <= 0))
					{
						stMmva.ObjFilterStatics[i].staytimeouts = 0;
						
					}
//					printf("i=%d  timeout=%d  pos [ %d %d %d %d] w=%d h=%d ",i,stMmva.ObjFilterStatics[i].staytimeouts,stMmva.ObjFilterStatics[i].left,stMmva.ObjFilterStatics[i].top,stMmva.ObjFilterStatics[i].right,stMmva.ObjFilterStatics[i].bottom,stMmva.ObjFilterStatics[i].w,stMmva.ObjFilterStatics[i].h);
			}
		}

	}

//	printf("%s------enter\n",__func__);

	return 0;



}
static void __mmva_hidecheck_alarm_report(BOOL bHideAlarm)
{
	int i;
	BOOL bReport;
	ALARMSET stAlarm;
	if(bHideAlarm)
	{
		if(stMmva.bHideAlarmStart == FALSE)
		{	_mmva_alram(NULL, 0, e_IPV_ALARM_TYPE_HIDE, NULL);	//每次都发报警信号
			stMmva.bHideAlarmStart = TRUE;

		}
		malarm_get_param(&stAlarm);
		stMmva.nAlarmtime = stAlarm.delay;
		if(stMmva.mva_tid > 0)
			utl_timer_reset(stMmva.mva_tid, stMmva.nAlarmtime*1000, __mmva_timer_callback, 0);

	}
}

static int mmva_hide_analysis(ObjHideStatics_t * stHide)
{	
	#define HIDE_THRESH_WIN  200
	U32 i,j,anv;
	U64 sum;
	BOOL bHide;
	BOOL bHideAlarm; 
	int thresholdWin;
	U32  delta[MAXWIN_W-2][MAXWIN_H-2];
	static int nHideCounts = 0;
	static int printcount = 0;
	if(!bInited)
		return 0;	
	if(delaywork < DELAYWORK_TIMEOUT)	//开机后200帧以内不工作
		return 0;		
	if(!stMivp.st_hide_attr.bHideEnable)
		return 0;
	bHide =FALSE;
	sum = 0;
	for(i=0;i<MAXWIN_H;i++)
		for(j=0;j<MAXWIN_W;j++)
			sum += stHide->statics[i][j];
		
	//归一化很重要,但是效果好像不咋地
	if(sum > 0)
	{
//		for(i=0;i<MAXWIN_H;i++)
//			for(j=0;j<MAXWIN_W;j++)
//				stHide->statics[i][j]= stHide->statics[i][j]*65536/sum;

		for(i=1;i<(MAXWIN_H-1);i++)
		{
			for(j=1;j<(MAXWIN_W-1);j++)
			{
			   	
				anv = stHide->statics[i-1][j-1] + stHide->statics[i-1][j+1] +stHide->statics[i+1][j-1] + stHide->statics[i+1][j+1];
				anv = anv/4;
				delta[i-1][j-1] = abs( stHide->statics[i][j] -anv);
//				printf("val=%d, anv=%d,delta=%d  \n ", stHide->statics[i][j], anv,delta[i-1][j-1] );
			}

		}

	}
	else
	{
		bHide =TRUE;
	}

	int threshold = 100 - stMivp.st_hide_attr.nThreshold;
	int nHideWin = 0;
	if(sum > 0)
	{
//		printf("delta= \n")	;
		for(i=0;i<MAXWIN_H-2;i++)
		{
			for(j=0;j<MAXWIN_W-2;j++)
			{
//				printf("%d \t", delta[i][j] );
				if(delta[i][j] < HIDE_THRESH_WIN)
					nHideWin++;
			}
// 			printf("\n");
		}
		if(threshold < 10)
			threshold = 10;
		thresholdWin = threshold*18/100 + 11;
		if( nHideWin >= thresholdWin)
		{
			bHide =TRUE;
		}

	}
	if(bHide)
		nHideCounts++;
	else
		nHideCounts = 0;
	
	if(nHideCounts > 8)	//发生遮挡报警
		bHideAlarm = TRUE;
	else
		bHideAlarm = FALSE;

#if 0
	if(++printcount > 16)
	{
		printcount = 0;
		printf("nHideWin=%d thresholdWin=%d threshval=%d\n",nHideWin,thresholdWin,HIDE_THRESH_WIN)	;
		for(i=0;i<MAXWIN_H-2;i++)
		{
			for(j=0;j<MAXWIN_W-2;j++)
			{
				printf("%d \t", delta[i][j] );
			}
 			printf("\n");
		}

	}
#endif

//	printf("bHideAlarm=%d ,nHideWin=%d  thresholdWin=%d\n ",bHideAlarm,nHideWin,thresholdWin);
	__mmva_hidecheck_alarm_report(bHideAlarm);


//	for(i=0;i<MAXWIN_H;i++)
//	{
//		for(j=0;j<MAXWIN_W;j++)
//		{
//			printf("%d \t", stHide->statics[i][j] );
//		}
//		printf("\n");
//	}

	return 0;



}

//vgs 画线获取运动物体
static int __mmva_get_obj_result(int channelid,Polygon_s* polygon,int * color, int maxcnt, int baseW, int baseH)
{
	int i,j,next;
	int cnt ;
	cnt = 0;
	if(!stMivp.st_rl_attr.bMarkObject)
		return 0;
	for (i = 0; i < IVE_MAX_REGION_NUM; i++)
	{
		if(stMmva.ObjFilterStatics[i].staytimeouts > 0)
		{
			polygon[cnt].nCnt = 4;
			polygon[cnt].stPoints[0].x = stMmva.ObjFilterStatics[i].left * baseW /ATK_VI_W;
			polygon[cnt].stPoints[0].y = stMmva.ObjFilterStatics[i].top * baseH/ATK_VI_H;
			polygon[cnt].stPoints[1].x = stMmva.ObjFilterStatics[i].right * baseW /ATK_VI_W;
			polygon[cnt].stPoints[1].y = stMmva.ObjFilterStatics[i].top * baseH /ATK_VI_H;
			polygon[cnt].stPoints[2].x = stMmva.ObjFilterStatics[i].right * baseW /ATK_VI_W;
			polygon[cnt].stPoints[2].y = stMmva.ObjFilterStatics[i].bottom* baseH /ATK_VI_H;
			polygon[cnt].stPoints[3].x = stMmva.ObjFilterStatics[i].left * baseW /ATK_VI_W;
			polygon[cnt].stPoints[3].y = stMmva.ObjFilterStatics[i].bottom* baseH /ATK_VI_H;
			for(j=0; j<4; j++)
			{
				polygon[cnt].stPoints[j].x = (polygon[cnt].stPoints[j].x>>1)<<1;
				polygon[cnt].stPoints[j].y = (polygon[cnt].stPoints[j].y>>1)<<1;
			}
			
			if(stMmva.ObjFilterStatics[i].w <2 || stMmva.ObjFilterStatics[i].h <2)
				continue;
			if(stMmva.ObjFilterStatics[i].bAlarmed)
			{
				color[cnt] = ALARM_COLOR;
			}
			else
			{
				color[cnt] = DEFAULT_COLOR;
				if(!stMivp.st_rl_attr.bMarkAll)
					continue;
			}
			cnt++;
			if(cnt>=maxcnt)
				break;
		}

	}

	return cnt;
}

//vgs画线获取区域绊线边框
static int __mmva_get_rgn_or_line(MIVP_t * mivp,int channelid,Polygon_s* polygon, int * color, int maxcnt, int baseW, int baseH)
{
	int i,j;
	int cnt ;
	int a,b,c,d;
	static int count[4]={0}; 
	if(mivp->st_rl_attr.bEnable != TRUE)
		return 0;
	if(mivp->st_rl_attr.bDrawFrame  != TRUE)
		return 0;
	if(mivp->st_rl_attr.nRgnCnt < 1)
		return 0;
	cnt = 0;
	for (i = 0; i < mivp->st_rl_attr.nRgnCnt; i++)
	{
		for(j=0; j < mivp->st_rl_attr.stRegion[i].nCnt ; j++)
		{
				polygon[cnt].stPoints[j].x = mivp->st_rl_attr.stRegion[i].stPoints[j].x * baseW /stMmva.viW;
				polygon[cnt].stPoints[j].y = mivp->st_rl_attr.stRegion[i].stPoints[j].y * baseH /stMmva.viH;
				polygon[cnt].stPoints[j].x = (polygon[i].stPoints[j].x>>1)<<1;
				polygon[cnt].stPoints[j].y = (polygon[i].stPoints[j].y>>1)<<1;
//				if(channelid==1 && i==1);
//				{
//					printf("error vgs :n=%d  x= %d y=%d\n ",j,polygon[cnt].stPoints[j].x,polygon[cnt].stPoints[j].y);
//				}
		}
		polygon[cnt].nCnt = mivp->st_rl_attr.stRegion[i].nCnt;

		//防止出现两个相同的点
		for(a=0;a < polygon[cnt].nCnt;a++)
		{
			b = (a+1)%polygon[cnt].nCnt;
			if((polygon[cnt].stPoints[a].x== polygon[cnt].stPoints[b].x)&&(polygon[cnt].stPoints[a].y== polygon[cnt].stPoints[b].y))
			{
				for(c=a;c < polygon[cnt].nCnt ;c++ )
				{
					d = (c+1)%polygon[cnt].nCnt;
					memcpy(&polygon[cnt].stPoints[c],&polygon[cnt].stPoints[d],sizeof(POINT_s));

				}
				polygon[cnt].nCnt--;
				a--;
			}
			
		}


		
		if(mivp->st_rl_attr.stRegion[i].nCnt < 2)
			continue;
		color[cnt] = REGION_LINE_COLOR;
		if(mivp->st_rl_attr.stRegion[i].flickertimeout> 0 )
		{	if( mivp->st_rl_attr.bFlushFrame)
			{
				count[i]++;
				if(count[i] < 5)
				{
					color[cnt] = ALARM_COLOR;
				}
				else if(count[i] > 10)
				{
					count[i]=0;
				}
			}
			else
			{
				color[cnt] = ALARM_COLOR;
			}
		}
		else
		{
			count[i] = 0;
		}
			
		cnt++;	
		if(cnt>=maxcnt)
			break;

	}

	return cnt;
}

//vgs画线获取计数视频线的位置
static int __mmva_get_countline(MIVP_t * mivp,int channelid,Polygon_s* polygon, int * color, int maxcnt, int baseW, int baseH)
{
	int j;
	int cnt ;
	if(mivp->st_count_attr.bOpenCount != TRUE)
		return 0;
	if( mivp->st_count_attr.bDrawFrame  != TRUE)
		return 0;
	if(mivp->st_count_attr.nPoints !=2)
		return 0;
	cnt = 0;
	for(j=0; j < mivp->st_count_attr.nPoints ; j++)
	{
			polygon[0].stPoints[j].x = mivp->st_count_attr.stPoints[j].x * baseW /stMmva.viW;
			polygon[0].stPoints[j].y = mivp->st_count_attr.stPoints[j].y * baseH /stMmva.viH;
			polygon[0].stPoints[j].x = (polygon[0].stPoints[j].x>>1)<<1;
			polygon[0].stPoints[j].y = (polygon[0].stPoints[j].y>>1)<<1;
	}
	polygon[0].nCnt = mivp->st_count_attr.nPoints;

	if((polygon[0].stPoints[0].x== polygon[0].stPoints[1].x)&&(polygon[0].stPoints[0].y== polygon[0].stPoints[1].y))
	{
		return cnt;
	}
	
	color[0] = LINE_COUNT_COLOR;

	cnt++;	


	return cnt;
}

//vgs画线获取爬高视频线的位置
static int __mmva_get_Climbline(MIVP_t * mivp,int channelid,Polygon_s* polygon, int * color, int maxcnt, int baseW, int baseH)
{
	int j;
	int cnt ;
	static int count = 0;
	if(mivp->st_rl_attr.bEnable != TRUE)
		return 0;
	if(mivp->st_rl_attr.bDrawFrame  != TRUE)
		return 0;
	if(mivp->st_rl_attr.stClimb.nPoints !=2)
		return 0;
	cnt = 0;
	for(j=0; j < mivp->st_rl_attr.stClimb.nPoints ; j++)
	{
			polygon[0].stPoints[j].x = mivp->st_rl_attr.stClimb.stPoints[j].x * baseW /stMmva.viW;
			polygon[0].stPoints[j].y = mivp->st_rl_attr.stClimb.stPoints[j].y * baseH /stMmva.viH;
			polygon[0].stPoints[j].x = (polygon[0].stPoints[j].x>>1)<<1;
			polygon[0].stPoints[j].y = (polygon[0].stPoints[j].y>>1)<<1;
	}
	polygon[0].nCnt = mivp->st_rl_attr.stClimb.nPoints;

	if((polygon[0].stPoints[0].x== polygon[0].stPoints[1].x)&&(polygon[0].stPoints[0].y== polygon[0].stPoints[1].y))
	{
		return cnt;
	}
	
	color[0] = CLIMB_UP_COLOR;
	if(mivp->st_rl_attr.stClimb.flickertimeout> 0 )
	{	if( mivp->st_rl_attr.bFlushFrame)
		{
			count++;
			if(count < 5)
			{
				color[cnt] = ALARM_COLOR;
			}
			else if(count > 10)
			{
				count=0;
			}
		}
		else
		{
			color[cnt] = ALARM_COLOR;
		}
	}
	else
	{
		count = 0;
	}



	cnt++;	


	return cnt;
}



//VGS模块视频画线的回调函数
static void mmva_get_graph(int channelid, GRAPH_t* graph, int* cnt, int basew, int baseh)
{
	int num;
	int grahpcnt = 0;
	int i;

	GRAPH_t* pGraph;
	int  color[32] ;
	Polygon_s polygon[32];
	if(!bInited)
	{
		*cnt = 0;
		return;	
	}
	if(delaywork < DELAYWORK_TIMEOUT)	//开机后200帧以内不工作
	{
		delaywork++;
		*cnt = 0;
		return;		
	}
	if(stMivp.st_rl_attr.bEnable)
	{
		//添加运动物体
		pGraph = graph;
		num = __mmva_get_obj_result(channelid,&polygon[0], color,ARRAY_SIZE(polygon), basew, baseh);
		for (i = 0; i < num; ++i)
		{
			// draw rect
			pGraph[i].graph_type = GRAPH_TYPE_POLYGON;
			pGraph[i].color =  color[i];
			pGraph[i].linew = (channelid == 0) ? 2 : 2;
			memcpy(&pGraph[i].Polygon,&polygon[i],sizeof(Polygon_s));
			grahpcnt++;
			if ((grahpcnt+1) >= *cnt)break;

		
		}
		pGraph = &graph[grahpcnt];
		
		//添加检测框和绊线
		num = __mmva_get_rgn_or_line(&stMivp,channelid,&polygon[0], color,ARRAY_SIZE(polygon), basew, baseh);
		for (i = 0; i < num; ++i)
		{
			pGraph[i].graph_type = GRAPH_TYPE_POLYGON;
			pGraph[i].color =  color[i];
			pGraph[i].linew = (channelid == 0) ? 4 : 2;
			memcpy(&pGraph[i].Polygon,&polygon[i],sizeof(Polygon_s));
			grahpcnt++;
			if ((grahpcnt+1) >= *cnt)break;

		}
		
		pGraph = &graph[grahpcnt];
		num = __mmva_get_Climbline(&stMivp,channelid,&polygon[0], color,ARRAY_SIZE(polygon), basew, baseh);
		for (i = 0; i < num; ++i)
		{
			pGraph[i].graph_type = GRAPH_TYPE_POLYGON;
			pGraph[i].color =  color[i];
			pGraph[i].linew = (channelid == 0) ? 4 : 2;
			memcpy(&pGraph[i].Polygon,&polygon[i],sizeof(Polygon_s));
			grahpcnt++;
			if ((grahpcnt+1) >= *cnt)break;

		}
		
	}
	if(stMmva.bLinecountSupport && stMivp.st_count_attr.bOpenCount)
	{
		pGraph = &graph[grahpcnt];
		num = __mmva_get_countline(&stMivp,channelid,&polygon[0], color,ARRAY_SIZE(polygon), basew, baseh);
		for (i = 0; i < num; ++i)
		{
			pGraph[i].graph_type = GRAPH_TYPE_POLYGON;
			pGraph[i].color =  color[i];
			pGraph[i].linew = (channelid == 0) ? 4 : 2;
			memcpy(&pGraph[i].Polygon,&polygon[i],sizeof(Polygon_s));
			grahpcnt++;
			if ((grahpcnt+1) >= *cnt)break;

		}
	
	}



	*cnt = grahpcnt;
//	printf("grahpcnt= %d\n",grahpcnt);

//	U64 b=utl_get_ms();
//	printf("get osd rect time over %lld\n",b-a);
}

//检查云台状态，判断是否需要挂起
static BOOL mmva_get_suspends(void)
{

	if(PTZ_Get_Status(0))		//运动时停止检测
		return TRUE;

	return FALSE;
}

//初始化小维mmva模块，设置portting层的回调
int mmva_init()
{

	memset(&stMmva,0,sizeof(stMMVA_t));
	stMmva.mva_tid = -1;
	//设置连通域规则检查回调
	jv_mva_register_ccl_analysis_callback(mmva_ccl_analysis);
	//设置VGS画框回调
	jv_stream_set_graph_callback(mmva_get_graph);
	//设置暂停挂起计算的回调
	jv_mva_register_getsuspends_callback(mmva_get_suspends);
	if(mivp_hide_bsupport())
		stMmva.bHideCheckSupport = TRUE;
	if(mivp_count_bsupport())
		stMmva.bLinecountSupport = TRUE;
	//攀高检测的判定
	if(mivp_climb_bsupport())
		stMmva.bClimbCheckSupport = TRUE;
	//设置遮挡计算回调
	if(stMmva.bHideCheckSupport)
		jv_mva_register_hide_analysis_callback(mmva_hide_analysis);
	
	bInited =TRUE;
	delaywork = 0;
	return jv_mva_init();
}
//小维智能分析模块MVA 去初始化
int mmva_deinit()
{
	if(stMmva.mva_tid > 0 )
		utl_timer_destroy(stMmva.mva_tid);
	stMmva.mva_tid = -1;
	return 0;
}
//复制参数
int mmva_push_param(MIVP_t *mivp)
{
	memcpy(&stMivp,mivp,sizeof(MIVP_t));
	return 0;
}
//启动mmva 模块
int mmva_start( MIVP_t *Mivp)
{
	int i;
	jvstream_ability_t ability;
	memcpy(&stMivp,Mivp,sizeof(MIVP_t));
	for(i=0;i<stMivp.st_rl_attr.nRgnCnt;i++)
	{
		stMivp.st_rl_attr.stRegion[i].eAlarmType = 0;
		stMivp.st_rl_attr.stRegion[i].staytimeout = 0;
	}
	stMmva.nAlarmtime = 20;

	jv_stream_get_ability(0, &ability);
	stMmva.viW = ability.inputRes.width;
	stMmva.viH = ability.inputRes.height;
	
	if((!stMivp.st_rl_attr.bEnable) && (!stMivp.st_count_attr.bOpenCount) && (!stMivp.st_hide_attr.bHideEnable))
		return 0;
	if(stMmva.mva_tid <= 0)
		stMmva.mva_tid = utl_timer_create("mva", 10*1000, __mmva_timer_callback, NULL);
	
	return jv_mva_start(0);
	
}


//关掉mmva模块
int mmva_stop()
{
	stMivp.st_rl_attr.bEnable = 0;
	stMivp.st_count_attr.bOpenCount = 0;
	if(stMmva.mva_tid > 0)
		utl_timer_reset(stMmva.mva_tid, 0, __mmva_timer_callback, 0);
	return jv_mva_stop(0);
}

//复位计数
int mmva_count_reset()
{

	stMmva.count_in = 0;
	stMmva.count_out = 0;
//	printf("enter %s \n",__func__);
	return 0;
}



//获取计数
int mmva_count_in_get()
{

	return  stMmva.count_in;

}

//获取计数
int mmva_count_out_get()
{
	return stMmva.count_out;

}

//mivp设置报警回调
int mmva_register_alarmcallbk( ON_IVP_ALARM callbk)
{
	_mmva_alram = callbk;
	return 0;

}
//设置灵敏度度，暂时无用
void  mmva_set_sensitivity(int nSensitivity)
{

//	stMmva.nSensitivity = nSensitivity;

}
//设置报警的持续时间,
//特指客户端收到报警开始到停止的持续时间。
void  mmva_set_alarmtime(int ntime)
{
	stMmva.nAlarmtime = ntime;
}
