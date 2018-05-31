#ifndef _M_PLAY_REMOTE_H
#define _M_PLAY_REMOTE_H


#include "jv_common.h"
#include "m_mp4.h"

typedef enum
{
	EN_PLAYER_TYPE_NORMAL,		//普通文件播放
	EN_PLAYER_TYPE_TIMEPOINT,	//根据时间点查找文件播放
} PlayerType_e;

typedef enum
{
	EN_PLAYER_MODE_ONE,			//单个播放,播放完成即停止
	EN_PLAYER_MODE_ONE_CYCLE,	//单个循环播放
	EN_PLAYER_MODE_SEQUENCE,	//顺序播放,默认按照时间排序
	EN_PLAYER_MODE_RANDOM,		//随机播放
} PlayerMode_e;

typedef enum
{
	EN_PLAYER_NO = 0,			//非回放状态
	EN_PLAYER_PLAY,				//播放
	EN_PLAYER_PAUSE,			//暂停
	EN_PLAYER_STEP,				//单帧
	EN_PLAYER_BACKSTEP,			//后退
	EN_PLAYER_BACKFAST,			//后退
	EN_PLAYER_SPEED,			//快进或快退
	EN_PLAYER_MAX,
} PlayerStatus_e;

//对于某些编码器，可以跳帧参考
//这样，回放时就可以丢弃一些帧来加速播放
typedef enum
{
	VDEC_SPEED_SKIP_NO = 0,		//不需要跳帧
	VDEC_SPEED_SKIP_ONLY_I = 1,	//只送I帧
	VDEC_SPEED_SKIP_2 = 2,		//2帧送一个
	VDEC_SPEED_SKIP_4 = 4,		//4帧送一个
	VDEC_SPEED_SKIP_8 = 8,		//8帧送一个
	VDEC_SPEED_SKIP_BUTT
	
} VDEC_SKIP_FRAME_E;

typedef struct tag_I_index
{
	S32	nFrames;		//关键帧的帧序号，是所有帧的序号
	S32	nFilePos;		//关键帧在文件中的位置
}I_Index;

typedef struct
{
	U32				clientId;		//分控id号
	U32				nConnectionType;//连接类型
	U64				msLast;			//上次调度的时间，单位ms，用作帧率控制
	U64				msWant;			//期望调度的时间，单位ms，用作帧率控制
	U64				msWantA;		//音频期望调度的时间，单位ms，用作帧率控制

//	BOOL			bMute;			//是否静音
	float 			speed;			//播放速度
	U32				nTimeSlice;		//帧间隔，单位ms
//	RECT_T 			posOutput;		//在屏幕上的输出位置
//	mplayer_status_callback_t callback;	//回调函数
	char				fname[256];		//要播放的文件名
//	char				nextfname[256];	//下一个要播放的文件
//	char				priorfname[256];	//下一个要播放的文件
//	BOOL			nextImmediately;		//马上更换下一个文件
	int				setPosInFrame;	//设置要播放的位置，单位为帧
	BOOL			IFrameBackward;	//I帧回退
	int				IFrameBackSpeed;	//后退速度。
										// 0 表示单帧播放，且已经播放，
										// 1 表示单帧播放，还有一帧需要播放
										// 4,8,16表示按8倍速，16倍速播放
	PlayerStatus_e	playStatus;			//播放状态：暂停、单步、快进之类的
	PlayerStatus_e	oldPlayStatus;		//暂停前的播放状态

	VDEC_SKIP_FRAME_E skipflag;		//跳帧快放：是否需要跳帧发送

	U32				frameRate;		//播放的原始帧率
	U32				frameDecoded;	//已解码帧数量
	BOOL 			bMp4;
	void 			*fp;
	U32				nWidth;
	U32				nHeight;
//======mp4使用的字段
	MP4_READ_INFO   mp4Info;
	BOOL 			bFileOver;
	U32				frameDecodedA;	//已经播放的音频帧,用来做帧率控制
//end
//======sv5使用的字段
//	FILE 			*fp;				//文件索引
	JVS_FILE_HEADER	header;			//文件头信息
	I_Index 		*I_IndexList;	//I帧列表，
	int 			I_Cnt;			//I帧个数
//end

	pthread_t thread;
	BOOL running;
	BOOL reqEnd;	//分控请求播放完成
//	BOOL created;

}RemotePlayer_t;

//远程回放初始化
void Remote_Player_Init();

//关闭远程回放
void Remote_Player_Release();

//创建远程回放播放器
//clientId			:分控id
//fname 			:要播放的文件名
//nConnectionType	:连接类型
//nSeekFrame        :定位到帧,和定位到时间点不能同时使用
//nSeekTime	        :定位到时间点
//return 0 成功，<0 失败
int Remote_Player_Create(S32 clientId, int nConnectionType, PlayerType_e playType, PlayerMode_e playMode, void *param);
int Remote_Tutk_Player_Create(S32 clientId, char *fname, int nConnectionType);

//关闭远程回放播放器
int Remote_Player_Destroy(S32 clientId);
int Remote_Tutk_Player_Destroy(S32 clientId);

//加速
int Remote_Player_Fast(S32 clientId);

//减速
int Remote_Player_Slow(S32 clientId);

//正常速度播放
int Remote_Player_PlayNormal(S32 clientId);

//暂停
int Remote_Player_Pause(S32 clientId);
int Remote_Tutk_Player_Pause(S32 clientId);


//取消暂停
int Remote_Player_Resume(S32 clientId);
int Remote_Tutk_Player_Resume(S32 clientId);

//设置播放位置
int Remote_Player_Seek(S32 clientId, U32 value);

//sv5的帧类型转换成云视通的帧类型
int Remote_Frametype_sv5_to_yst(S32 typeStorage);

//读取mp4文件信息
int Remote_ReadFileInfo_MP4(RemotePlayer_t *player);
//读取mp4帧
BOOL Remote_Read_Frame_MP4(void *handle, MP4_READ_INFO *pInfo, PAV_UNPKT pPack);

//time时间轴录像回放
typedef struct
{
	int clientId;
	int connectType;
	unsigned int play_time;
	unsigned int base_time;
}REPLAY_INFO;

int Remote_Player_Time_Destroy(S32 clientId);
void Remote_Player_Time(S32 clientId, int nConnectionType, unsigned int play_time,unsigned long long base_time);


//qq物联远程回放
void Remote_play_history_video(int channelno,char* filename,int starttime,int timeoffset);
int Remote_Destroy_history_video();

#endif

