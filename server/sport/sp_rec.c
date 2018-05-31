#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#include <SYSFuncs.h>

#include "sp_define.h"
#include "sp_rec.h"
#include "mplay_remote.h"
#include "mrecord.h"

/**
 * 播放结构
 */
typedef struct
{
	FILE* 	fp_ps;             		//ps文件标识符
	FILE* 	fp_index;				//解析结果文件标识符
//	void 	*fp;
	int 	ret;					//文件读取结果检查
	int 	framecount;
	int 	channel_id;				//通道id
	long 	rec_start_time;			//录像播放开始时间
	long 	rec_end_time;			//录像播放结束时间s
} REC_Play;

/**
 *ps文件解析结构
 */
typedef struct
{
	int i;						//h264帧位置
	int len;					//帧长度
	RtpPackType_e frame_type;	//帧类型
} psFRAME_INFO;

RemotePlayer_t player;

//文件检索
#define MOUNT_PATH		"./rec/00/"
#define	FILE_FLAG		".mp4"
static void _CheckRecFile(char chStartTime[14], char chEndTime[14], char *pBuffer, int *nSize)
{
    char strT[6]={0};
	char strPath[128]={0};
	char strFolder[10]={0};
	U8	TypePlay;		//搜索到的录像文件类型
	U32	i = 0, ChnPlay;	//搜索到的录像文件通道

	*nSize=0;
	sprintf(strPath, "%s/", MOUNT_PATH);//"/mnt/rec",i);

	if(access(strPath, F_OK))
	{
		return;//如果分区不存在则终止搜索
	}
	memset(strFolder, 0, 10);
	strncat(strFolder, chStartTime, 8);
	strcat(strPath, strFolder);

	if(access(strPath, F_OK))
	{
		Printf("Path:%s can't access\n", strPath);
		return;//如果文件夹不存在则继续搜索下一分区
	}
	DIR	*pDir	= opendir(strPath);
	struct dirent *pDirent	= NULL;
	while((pDirent=readdir(pDir)) != NULL)
	{
		//在这里限制搜索类型和通道
		if(!strcmp(FILE_FLAG, pDirent->d_name+strlen(pDirent->d_name)-strlen(FILE_FLAG)))
		{
			memcpy(&pBuffer[*nSize],pDirent->d_name,9);
			*nSize += 9;
		}
	}
	pBuffer[*nSize]=0;
	closedir(pDir);
	return;
}

/**
 *@brief 根据文件时间找文件路径
 *
 *@param file_time 文件的时间
 *@param filepath 文件的路径
 */
int _time2filename(long file_time,char *filepath)
{
	if(!filepath)
		return -1;
	
	char strPath[128]={0};
	char strFolder[10]={0};
	char chFileDate[10]={0};
	char chFileTime[10]={0};
	char temp[20]={0};
	char filename[20]={0};
	struct tm p_tm;
	memcpy(&p_tm,localtime((time_t *)&file_time),sizeof(struct tm));
	sprintf(chFileDate,"%.4d%.2d%.2d",p_tm.tm_year+1900,p_tm.tm_mon+1,p_tm.tm_mday);
	sprintf(chFileTime,"%.2d%.2d%.2d",p_tm.tm_hour,p_tm.tm_min,p_tm.tm_sec);
	sprintf(strPath, "%s", MOUNT_PATH);
	if(access(strPath, F_OK))
	{
		return -1;//如果分区不存在则终止搜索
	}
	memset(strFolder, 0, 10);
	strncat(strFolder, chFileDate, 8);
	strcat(strPath, strFolder);

	if(access(strPath, F_OK))
	{
		Printf("Path:%s can't access\n", strPath);
		return -1;//如果文件夹不存在则继续搜索下一分区
	}
	DIR	*pDir	= opendir(strPath);
	struct dirent *pDirent	= NULL;
	strncat(temp, chFileTime, 6);
	strcat(temp,FILE_FLAG);
	while((pDirent=readdir(pDir)) != NULL)
	{
		if(!strcmp(temp, pDirent->d_name+strlen(pDirent->d_name)-strlen(FILE_FLAG)-strlen(chFileTime)))
		{
			strcpy(filename,pDirent->d_name);
			break;
		}
	}
	strcat(filepath,strPath);
	strcat(filepath,"/");
	strcat(filepath,filename);
	closedir(pDir);
	return 0;
}
/**
 *@brief 暂停录像播放
 *
 *@param channelid 通道号
 */
int sp_rec_pause(int channelid)
{
	return 0;
}
/**
 *@brief 开始录像
 *
 *@param channelid 通道号
 */
int sp_rec_start(int channelid)
{
	__FUNC_DBG__();
#ifdef GB28181_SUPPORT
	mrecord_attr_t record;
	mrecord_get_param(0, &record);
	if(!record.bEnable)
	{
		record.bEnable = TRUE;
		mrecord_set_param(0, &record);
		mrecord_flush(0);
		WriteConfigInfo();
	}
#endif
	return 0;
}
/**
 *@brief 恢复播放录像
 *
 *@param channelid 通道号
 */
int sp_rec_resume(int channelid)
{
	return 0;
}

/**
 *@brief 结束播放录像
 *
 *@param channelid 通道号
 */
int sp_rec_stop(int channelid)
{
#ifdef GB28181_SUPPORT
	mrecord_attr_t record;
	mrecord_get_param(0, &record);
	if (record.bEnable)
	{
		record.bEnable = FALSE;
		mrecord_set_param(0, &record);
		mrecord_flush(0);
		WriteConfigInfo();
	}
#endif
	return 0;
}

/**
 *@brief 检查是否正在录像
 */
int sp_rec_b_on(int channelid)
{
	__FUNC_DBG__();
	mrecord_attr_t record;
	mrecord_get_param(0, &record);
	if(!record.bEnable)
	{
		record.bEnable = TRUE;
		mrecord_set_param(0, &record);
		mrecord_flush(0);
		WriteConfigInfo();
	}
	return 0;
}

/**
 * @brief 创建句柄打开文件
 */
REC_HANDLE sp_rec_create(int channelid, long start_time, long end_time)
{
	char filename[128]={0};
	struct tm p_tm;
	memcpy(&p_tm,localtime((time_t *)&start_time),sizeof(struct tm));
	sprintf(filename,MOUNT_PATH"%.4d%.2d%.2d/N01%.2d%.2d%.2d"FILE_FLAG,p_tm.tm_year+1900,p_tm.tm_mon+1,p_tm.tm_mday,p_tm.tm_hour,p_tm.tm_min,p_tm.tm_sec);

#ifdef GB28181_SUPPORT


	REC_Play *rec_play = (REC_Play *) malloc(sizeof(REC_Play));
	memset(rec_play, 0, sizeof(REC_Play));

	memset(&player, 0, sizeof(RemotePlayer_t));
	player.frameRate = 25;//先设置一个初始值
	strcpy(player.fname, filename);
//	strcpy(player.fname, MOUNT_PATH"20150114/N01111341"FILE_FLAG);

	rec_play->ret = Remote_ReadFileInfo_MP4(&player);
	rec_play->framecount = 0;
	rec_play->channel_id = channelid;
	rec_play->rec_start_time = start_time;
	rec_play->rec_end_time = end_time;
	rec_play->fp_ps = NULL;
	rec_play->fp_index = NULL;
//	rec_play->fp = player->fp;
	return rec_play;
#endif


	return 0;
}

/**
 *@brief 销毁rec句柄,关掉文件
 */
int sp_rec_destroy(REC_HANDLE handle)
{
	REC_Play *rec_play = (REC_Play *) handle;
	free(handle);

	return 0;
}

/**
 * @brief 检索文件
 *
 * @param channelid 通道号
 * @param start_time 开始时间
 * @param end_time 结束时间
 * @param search_file 返回结果
 */
REC_Search *sp_rec_search(int channelid, long start_time, long end_time)
{
	REC_Search *search_file, *list = NULL;
	int i=0;
	char buf[2048]={0};
	int nSize=0;
	char charstart_time[10]={0};
	char charend_time[10]={0};
	char rectype;
	char tmp[5]={0};
	int h,m,s;
	struct tm start_tm;
	struct tm end_tm;
	long start_time_day=0;
	memcpy(&start_tm,localtime((time_t *)&start_time),sizeof(struct tm));
	memcpy(&end_tm,localtime((time_t *)&end_time),sizeof(struct tm));
	
	Printf("%s,%d,%d,%d,%d,%d,%d\n",__func__,start_tm.tm_year+1900,start_tm.tm_mon+1,start_tm.tm_mday,start_tm.tm_hour,start_tm.tm_min,start_tm.tm_sec);
	Printf("%s,%d,%d,%d,%d,%d,%d\n",__func__,end_tm.tm_year+1900,end_tm.tm_mon+1,end_tm.tm_mday,end_tm.tm_hour,end_tm.tm_min,end_tm.tm_sec);

	strftime(charstart_time, sizeof(charstart_time), "%Y%m%d", &start_tm);
	strftime(charend_time, sizeof(charend_time), "%Y%m%d", &end_tm);
	_CheckRecFile(charstart_time,charend_time,buf,&nSize);

	//这一天刚开始时的秒数
	start_tm.tm_sec=0;
	start_tm.tm_min=0;
	start_tm.tm_hour=0;
	start_time_day=mktime(&start_tm);

	if(nSize==0)
	{
		printf("_CheckRecFile none\n");
		return NULL;
	}
	else
	{
		for(i=0;i<(nSize/9);i++)
		{
			search_file = malloc(sizeof(REC_Search));
			if(search_file==NULL)
			{
				Printf("malloc error\n");
				break;
			}
			strncpy(search_file->fname,buf+i*9,9);		//文件路径
			strcpy(search_file->fname+9,".mp4");
			Printf("file name=%s ",search_file->fname);
			sscanf(search_file->fname, "%c%2s%2d%2d%2d%4s",&rectype,tmp, &h,&m,&s,tmp);
			Printf("file type=%c,file time=%.2d%.2d%.2d\n",rectype,h,m,s);
			
			search_file->start_time = start_time_day+h*60*60+m*60+s;	//录像文件开始时间
			search_file->end_time   = start_time_day+h*60*60+m*60+s+60*3;//录像文件结束时间
			search_file->secrecy    = 0;							//保密属性/0为不涉密/1为涉密
			search_file->type       = (rectype=='M'?SP_REC_TYPE_MOTION:SP_REC_TYPE_ALL); //录像产生类型
			search_file->next       = list;
			list = search_file;
		}
	}
	return list;
}

/**
 * @brief 删除检索信息
 *
 * @param list 搜索结果。#sp_rec_search 的结果
 *
 * @return 检索结果输出
 *
 */
int sp_rec_search_release(REC_Search *list)
{
	REC_Search *p = list;
	REC_Search *q;
	while (p)
	{
		q = p;
		p = p->next;
		free(q);
	}
	return 0;
}

/**
 * @brief 检索信息数量
 *
 * @param list 搜索结果。#sp_rec_search 的结果
 *
 * @return 检索结果数量输出
 *
 */
int sp_rec_search_cnt(REC_Search *list)
{
	REC_Search *p = list;
	int cnt=0;
	while (p)
	{
		p = p->next;
		cnt++;
	}
	return cnt;
}

/**
 * @brief 定位视频播放位置
 */
int sp_rec_seek(REC_HANDLE handle, int play_range_begin)
{
	return 0;
}

/**
 * @brief 读取ps文件返回原始帧，每掉用一次返回1帧
 */
int sp_rec_read(REC_HANDLE handle, FrameInfo_t *pBuf)
{
#ifdef GB28181_SUPPORT
//#if 1
	int last_len = 0, i = 0;
	REC_Play *rec_play = (REC_Play *) handle;

	AV_UNPKT pack;
	pack.iType = JVS_UPKT_VIDEO;
	pack.iSampleId = player.frameDecoded + 1;

	Remote_Read_Frame_MP4(player.fp, &player.mp4Info, &pack);
	if(pack.iSize)
		player.frameDecoded++;
	if(!pack.iSize)
	{
		player.bFileOver = TRUE;
		return -1;	//退出播放
	}
	if (pack.bKeyFrame)
	{
		pBuf->type = RTP_PACK_TYPE_FRAME_I;
	}
	else
	{
		pBuf->type = RTP_PACK_TYPE_FRAME_P;
	}

	memcpy(pBuf->buf,pack.pData,pack.iSize);
	pBuf->len = pack.iSize;
	pBuf->framerate = (U32)player.mp4Info.dFrameRate;			//帧率
	pBuf->width = player.mp4Info.iFrameWidth;;					//帧高
	pBuf->height = player.mp4Info.iFrameHeight;				//帧宽
#endif
	return 0;
}

//////////////////////////tutk//////////////////////////////////
/**
 * @brief 设置录像模式
 *
 * @param channelid 通道号
 * @param rectype 录像模式
 *
 * @return 0 或者是错误号
 */
int sp_rec_set_mode(int channelid, SPRecType_e rectype)
{
	mrecord_attr_t record;
	int ret;
	mrecord_get_param(channelid, &record);
	switch(rectype)
	{
		case SP_REC_TYPE_MOTION:
			record.alarm_enable=TRUE;
			record.detecting=TRUE;
			record.bEnable=FALSE;
			break;
		case SP_REC_TYPE_MANUAL:
			record.bEnable=TRUE;
			record.alarm_enable=FALSE;
			record.detecting=FALSE;
			break;
		case SP_REC_TYPE_STOP:
			record.detecting     = FALSE;
			record.bEnable       = FALSE;
			record.timing_enable = FALSE;
			record.alarm_enable  = FALSE;
			record.discon_enable = FALSE;
			break;
		default:
			break;
	}
	ret=mrecord_set_param(channelid, &record);
	ret|=mrecord_flush(0);
	WriteConfigInfo();
	return ret;
}

/**
 * @brief 获得录像模式
 *
 * @param channelid 通道号
 *
 * @return 录像模式
 */
SPRecType_e sp_rec_get_mode(int channelid)
{
	mrecord_attr_t record;
	mrecord_get_param(channelid, &record);
	if(record.detecting==TRUE && record.alarm_enable==TRUE)
	{
		return SP_REC_TYPE_MOTION;
	}
	else if(record.bEnable==TRUE)
	{
		return SP_REC_TYPE_MANUAL;
	}
	else if(record.detecting==FALSE && record.bEnable ==FALSE && record.timing_enable ==FALSE && record.alarm_enable == FALSE && record.discon_enable == FALSE)
	{
		return SP_REC_TYPE_STOP;
	}
	else
	{
		return SP_REC_TYPE_MANUAL;
	}
}

unsigned int sp_rec_set_length(int channelid, unsigned int len)
{
	mrecord_attr_t record;
	mrecord_get_param(channelid, &record);
	if(len!=record.file_length)
	{
		record.file_length = len;
		mrecord_set_param(channelid, &record);
		mrecord_flush(0);
		WriteConfigInfo();
	}
	return len;
}
unsigned int sp_rec_get_length(int channelid)
{
	mrecord_attr_t record;
	mrecord_get_param(channelid, &record);
	return record.file_length;
}
