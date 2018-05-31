#ifndef MBIZCLIENT_H
#define MBIZCLIENT_H

typedef struct
{
	char		yst_no[32]; 		/* 云视通号码*/
	char		dev_type[32];		/* 设备类型 */
	char		dev_version[32];	/* 设备版本 */
	char		username[32];		/* 用户名 */
	char		password[32];		/* 密码 */
	char		dev_chans;			/* 设备通道数量 */
	char		sdcard; 			/* 是否存在SD卡0 不存在 1存在 */
	char		cloud;				/* 是否支持云存储 0-不支持 1-支持 */
	char		netlibversionstr[60];	/* 网络库版本 */
	U32 		netlibversion;			/* 网络库版本 */
}BizDevInfo;

int mbizclient_init();

int mbizclient_updateuserinfo_server(char *username, char *password);

int mbizclient_PushDevInfo(BizDevInfo *info, BOOL bModifyUserInfo);

/** 
 *	@brief 推送报警信息
 *	@param yst_no	 报警云视通号
 *	@param channel	 报警通道
 *	@param amt		 报警消息类型 0文本消息，1图片上传完成，2视频上传完成。对于云视通方案为0)
 *	@param aguid	 报警GUID
 *	@param atype	 报警类型
 *	@param atime	 报警时间
 *	@param apicture  报警图片地址
 *	@param avideo	 报警视频地址
 *	@return ZUINT 0 成功 -1 失败.
 */
int mbizclient_PushAlarm(char* devname, char* yst_no, U8 channel, U8 amt,
							char* aguid, U8 atype, U32 atime, char* apicture, char* avideo);

#endif