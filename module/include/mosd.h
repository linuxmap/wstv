


/*	mosd.h
	Copyright (C) 2011 Jovision Technology Co., Ltd.
	此文件用来组织OSD显示相关的功能，模块内只保存运行值,不直接用作和用户交互,lck20120110
	更改历史详见svn版本库日志
*/

#ifndef __MOSD_H__
#define __MOSD_H__

#define	RGN_WIDTH		240
#define	RGN_HEIGHT		60
#define FONT_LINE_PAD	4
#define FONT_HEIGHT		osdstatus.flheader->YSize
#define	TEXT_LINE_NUM	8
typedef enum{
	MCHNOSD_POS_LEFT_TOP=0,
	MCHNOSD_POS_LEFT_BOTTOM,
	MCHNOSD_POS_RIGHT_TOP,
	MCHNOSD_POS_RIGHT_BOTTOM,
	MCHNOSD_POS_HIDE,
}mchnosd_pos_e;
typedef enum{
	MULTIOSD_POS_LEFT_TOP=0,
	MULTIOSD_POS_LEFT_BOTTOM,
	MULTIOSD_POS_RIGHT_TOP,
	MULTIOSD_POS_RIGHT_BOTTOM,
	MULTIOSD_POS_HIDE,
}multiosd_pos_e;
typedef enum{
	MULTIOSD_LEFT=0,
	MULTIOSD_RIGHT,
}multiosd_align_e;

/**
 *@brief 时间格式定义
 *
 */
//typedef enum{
//	OSD_TM_FMT_MMDDYYYY,	///< 格式为05/07/2010
//	OSD_TM_FMT_YYYYMMDD,	///<格式为2010-05-07
//	OSD_TM_FMT_MAX
//}mosd_time_format_e;

/**
 *@brief channel osd status
 *
 */
typedef struct 
{
	BOOL				bShowOSD;			///< 是否在通道中显示OSD
	//mosd_time_format_e	tm_fmt;				///< 时间格式
	char					timeFormat[32];		///< 时间格式 YYYY-MM-DD hh:mm:ss 可随意组合
	mchnosd_pos_e		position;			///< OSD的位置 0, 左上，1，左下，2，右上，3，右下
	mchnosd_pos_e		timePos;			///< OSD的位置，时间部分 0, 左上，1，左下，2，右上，3，右下
	char					channelName[32];	///<通道名称
	BOOL osdbInvColEn;		//是否反色lk20131218
	BOOL bLargeOSD;			//是否用超大OSD
}mchnosd_attr;

/**
 *@brief channel osd section for GQ 
 *
 */
typedef struct 
{
	BOOL bEnable;
	unsigned int backColor;
	unsigned int fontColor;
	int posX;
	int posY;
	int text_line_num;
	char text[TEXT_LINE_NUM][256];
}mchnosd_section_attr;

/**
 *@brief 初始化
 *
 */
int mchnosd_init(void);

/**
 *@brief 结束
 *
 *
 */
int mchnosd_deinit(void);

/**
 *@brief 将时间（秒数）转换成指定格式的字符串
 *
 *@param timeFormat 日期格式，如：YYYY-MM-DD hh:mm:ss。Y代表年，M代表月依次类推
 * 				顺序随意，例如，也可以YYYY年MM月DD日hh:mm:ss
 *@param nsecond 秒数，time(NULL)的返回值
 *@param str 用于保存日期时间的内存空间
 *
 *@return str的地址
 *
 */
char *mchnosd_time2str(char *timeFormat, time_t nsecond, char *str);

/**
 *@brief 设置OSD的参数
 *
 *@param channelid 通道号
 *@param attr 要设置的属性
 *
 *@return 0 成功
 *
 */
int mchnosd_set_param(int channelid, mchnosd_attr *attr);

/**
 *@brief 获取OSD的参数
 *
 *@param channelid 通道号
 *@param attr 用于存储要获取的属性的指针
 *
 *@return 0 成功
 *
 */
int mchnosd_get_param(int channelid, mchnosd_attr *attr);


/**
 *@brief 停止OSD
 *
 *@param channelid 通道号
 *
 *@return 0 成功
 *
 */
int mchnosd_stop(int channelid);

/**
 *@brief 刷新，使设置生效
 *
 *@param channelid 通道号
 *
 *@return 0 成功
 *
 */
int mchnosd_flush(int channelid);

/**
 *@brief 设置通道号的名称
 *
 *@param channelid 通道号，-1表示所有通道
 *
 *@return 0 成功
 *
 */
int mchnosd_set_name(int channelid, char *strName);

/**
 *@brief 修改osd是否反色
 *
 *@param channelid 通道号，-1表示所有通道
 *
 *@return 0 成功
 *
 */
int mchnosd_set_be_invcol(int channelid,BOOL bInvColEn);

/**
 *@brief 设置通道号的名称
 *
 *@param channelid 通道号，-1表示所有通道
 *
 *@return 0 成功
 *
 */
int mchnosd_set_time_format(int channelid, char *timeFormat);

//设置是否显示调焦参考，帮助调试之用
void mchnosd_display_focus_reference_value(BOOL bDisplay);

//设置调试模式要显示的内容
void mchnosd_debug_mode(BOOL bDebug, char *display);

typedef struct{
	multiosd_pos_e attrPos;
	multiosd_align_e attrAlign;
	int multiOsdHandle;
}multiosd_info_t;

typedef struct{
	int x;	//pos
	int y;	//pos
	int endx;
	int endy;
	int rows;	//行数
	int columns;	//列数，即每行的字数
	int mainFontSize;
	int subFontSize;
	char text[TEXT_LINE_NUM][256];
	int text_w[3][TEXT_LINE_NUM];
	int max_w[3];
}mchnosd_region_t;

/*
 *@brief 创建用户自定义显示区域
 *
 *@param channelid 通道号，-1表示所有通道
 *@param param 区域大小等参数
 */
int mchnosd_region_create(int channelid, mchnosd_region_t *region);

int mchosd_region_stop();

/*
 *@brief 设置多行显示信息
 *
*/
int multiple_set_param(int mposition,int malignment);

/*
 *@brief 销毁用户自定义显示区域
 *
 *@param handle region的句柄,  #mchnosd_region_create 的返回值
 */
int mchnosd_region_destroy(int handle);

/**
 *@brief
 *
 *@param handle region的句柄， #mchnosd_region_create 的返回值
 *@param draw
 */
int mchnosd_region_draw(int handle, mchnosd_section_attr *draw);
/**
*智能分析人群密度显示
**/
int mchnosd_ivp_cde_draw(int rate);
/**
*智能分析刷新
**/
int __chnosd_ivpcde_flush(int channelid);

/**
 *@brief
 *@param 获取是否是OSD时间分开模式
 *
 */
BOOL mchnosd_get_seperate();

//以下为预置点和变倍显示定义信息
typedef struct
{
	int osdhandle[MAX_STREAM];
	int fontsize[MAX_STREAM];

	pthread_mutex_t mutex;
	pthread_t thread;
	BOOL bNeedRefresh;  //刷新标志
	BOOL bRunning;
	BOOL bZoomCalling;  //变倍调用标志
	BOOL bPresCalling;  //预置点调用标志
	int preset;         //预置点号
	char content[24];   //显示内容数组
	BOOL bOSDHide;      //隐藏标志(含OSD手动设置"隐藏"或10S自动隐藏)
	BOOL bNoMove;       //再无移动标志，用于10S自动隐藏
	BOOL bNoChange;     //显示内容无变化标志，用于10S自动隐藏
	
} ptzosdstatus_t;

ptzosdstatus_t *mosd_get_ptzstatus();

int mchnosd_draw_text(int channelid, int osdHandle, int px, int py, char *text, int fontsize);
/****************************************************************************************************************/
/*
 * 设置抓图osd的字体大小
 */
int mosd_set_snap_flush(int fontsize);
int mchnosd_snap_add_txt(int id,char *str);
int mchnosd_snap_del_txt(int id);
/****************************************************************************************************************/

/****************************************************************************************************************/
/*
 * 云台菜单
 */
#define VSC_OSD_CNT 3  //云台菜单显示3个码流
#define OSD_MAX_ROW 12	//最大12行
#define OSD_MAX_COL 26	//1行支持12个中文汉字，实际只显示11个

typedef struct
{
	BOOL newPage;	//新的一页
	int change[2];	//旧页，改变的行号:1行或2行
	char text[OSD_MAX_ROW][OSD_MAX_COL]; //接收字符串
}mvisca_osd_attr;

int mvisca_osd_copy(mvisca_osd_attr *from, int len);	//从接收缓冲区拷贝到显示缓存区
int mvisca_osd_build(int channelid);					//创建云台菜单区域
int mvisca_osd_flush(int channelid);					//修改码流刷新菜单区域
/****************************************************************************************************************/

#endif

