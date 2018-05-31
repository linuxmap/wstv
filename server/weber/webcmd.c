
/*
这里是一些测试用的命令：
./bin/wagent webalarm set '{"delay":10,"sender":"ipcmail@163.com","server":"smtp.163.com","username":"ipcmail","passwd":"ipcam71a","receiver0":"lfx@jovision.com","receiver1":"lfx1@jovision.com"}'
./bin/wagent webalarm list
./bin/wagent webmdetect 1 set '{"bEnable":0,"nSensitivity":49,"nThreshold":15,"nRectNum":1,"stRect":[{"x":10,"y":10,"w":20,"h":20},{"x":0,"y":0,"w":0,"h":0},{"x":0,"y":0,"w":0,"h":0},{"x":0,"y":0,"w":0,"h":0}],"nDelay":10,"nStart":0,"bOutClient":0,"bOutEMail":0}'
./bin/wagent webmdetect 1 list
./bin/wagent webprivacy 1 set '{"bEnable":0,"stRect":[{"x":10,"y":10,"w":30,"h":30},{"x":0,"y":0,"w":0,"h":0},{"x":0,"y":0,"w":0,"h":0},{"x":30,"y":40,"w":10,"h":20}]}'
./bin/wagent webprivacy 1 list
./bin/wagent webrecord 1 set '{"bEnable":0,"file_length":1800,"timing_enable":1230,"timing_start":110,"timing_stop":0,"detecting":0,"alarming":0}'
./bin/wagent webrecord 1 list
./bin/wagent webstream 1 set '{"bEnable":1,"width":1920,"height":1080,"framerate":30,"nGOP":50,"bitrate":2048}'
./bin/wagent webstream 1 list
*/


#include <mstream.h>
#include <mdetect.h>
#include <malarmin.h>
#include <mprivacy.h>
#include <malarmout.h>
#include <mrecord.h>
#include "msnapshot.h"
#include "maccount.h"
#include "mlog.h"
#include "mosd.h"
#include "mstorage.h"
#include "mipcinfo.h"
#include "msensor.h"
#include <utl_cmd.h>
#include "../sctrl/SYSFuncs.h"
#include "../sctrl/sctrl.h"
#include "../yst_svr/mtransmit.h"
#include <cJSON_Direct.h>
#include <utl_ifconfig.h>
#include <utl_iconv.h>
#include <jv_isp.h>
#include "mptz.h"
#include <math.h>

#include "webcmd.h"
#include "weber.h"
#include "sp_image.h"
#include "sp_stream.h"
#include "sp_ifconfig.h"
#include "sp_osd.h"
#include "sp_devinfo.h"
#include "sp_audio.h"
#include "sp_storage.h"
#include "sp_privacy.h"
#include "sp_mdetect.h"
#include "sp_alarm.h"
#include "sp_connect.h"
#include "sp_snapshot.h"
#include "jv_ao.h"
#include "jv_ai.h"
#include "jv_io.h"
#include <utl_timer.h>

int CheckPower = 0;

#define MAX_DIR_LEN 9         //20150818
#define MAX_EVENT_LEN 14      //N01185036.mp4

typedef struct _REC_FILE_NAME
{
	char name[MAX_EVENT_LEN];	//N01185036.mp4
}rec_file_t, *prec_file_t;

typedef struct _REC_FILE_LIST
{
	char 		date[MAX_DIR_LEN];	//20150101
	int  		cnt;		//file cnt
	char 		*name;		//file name
}rec_file_list_t, *prec_file_list_t;

static json_kinfo_t json_file_name[] =
{
	MAKE_KEY_INFO(rec_file_t, KEY_TYPE_STRING, name, NULL),

	MAKE_END_INFO()
};

static json_kinfo_t json_rec_file_list[] =
{
	MAKE_KEY_INFO(rec_file_list_t, KEY_TYPE_STRING, date 	, NULL),
	MAKE_KEY_INFO(rec_file_list_t, KEY_TYPE_U32, 	cnt	, NULL),
	MAKE_KEY_INFO(rec_file_list_t, KEY_TYPE_STRING_PTR, name, NULL),

	MAKE_END_INFO()
};


typedef struct __WEBRESULT
{
	char *status;
	char *data;
}WEBRESULT;
static json_kinfo_t json_webresult[] =
{
	MAKE_KEY_INFO(WEBRESULT, KEY_TYPE_STRING, status, NULL),
	MAKE_KEY_INFO(WEBRESULT, KEY_TYPE_STRING, data, NULL),

	MAKE_END_INFO()
};

void weber_format_result(WEBRESULT *result)
{
	weber_set_result(cjson_object2string(json_webresult,result));
}

//确认操作所需要的权限，管理员或者普通用户，原则是设置类的必须管理员权限，查询类的普通用户即可
Oper_Type_e _webcmd_checkOperation(char *cmd, char *action)
{
	if(NULL == action)  //websnapshot & webhelp
	{
		if(strcmp(cmd,"webhelp") == 0)
		{
			return OPER_NEED_USER;
		}
	}
	else if(strcmp(action,"list") == 0  //基本各类cmd都有list
			|| strcmp(action,"resolution") == 0  //webstream
			|| strcmp(action,"ability") == 0  //webstream
			|| strcmp(action,"get_port") == 0  //yst
			|| strcmp(action,"get_video") == 0  //yst
			|| strcmp(action,"imageget") == 0)  //multimedia
	{
		return OPER_NEED_USER;
	}
	else
	{
		return OPER_NEED_ADMIN;
	}
	return OPER_ERROR;
}

int _webcmd_checkpower(char *cmd, char *action, char *id, char *passwd)
{
	int ret;
	Oper_Type_e operType = OPER_ERROR;

	//目前只做LSK518产品，其他的不加用户鉴权
	if(CheckPower == 0)
	{
		return 0;
	}
	
	//action有为空的情况:cmd为websnapshot或者webhelp时
	if(NULL == cmd || NULL == id || NULL == passwd)
	{
		weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
		printf("pointers can't be null \n");
		return -1;
	}

	operType = _webcmd_checkOperation(cmd,action);

	ret = maccount_check_power_ex(id,passwd);
	if(ret==ERR_NOTEXIST_EX || ret==ERR_PASSWD_EX)
	{
		weber_set_result("{\"status\":\"username or password error\",\"data\":\"\"}");
		printf("username or password error \n");
		return -1;
	}
	//if(ret==POWER_GUEST || ret == (POWER_GUEST|POWER_FIXED))
	if(OPER_NEED_ADMIN == operType)
	{
		if(ret != POWER_ADMIN && ret != (POWER_ADMIN|POWER_FIXED))
		{
			weber_set_result("{\"status\":\"without permission\",\"data\":\"\"}");
			printf("user %s without permission \n",id);
			return -1;
		}
	}
	else if(OPER_NEED_USER == operType)
	{
		if(ret==POWER_GUEST || ret == (POWER_GUEST|POWER_FIXED))
		{
			weber_set_result("{\"status\":\"without permission\",\"data\":\"\"}");
			printf("user %s without permission \n",id);
			return -1;
		}
	}
	else
	{
		weber_set_result("{\"status\":\"current user has no this oper power \",\"data\":\"\"}");
		printf("current user has no oper power \n");
		return -1;
	}
	return 0;
}

//释放list的内存
void _webcmd_freeListIfNecessary(prec_file_list_t list, int dir_num)
{
	int i = 0;
	
	if(NULL == list)
	{
		return;
	}
	//遍历list内每个元素的name指针，如果不为空先释放name
	for(i = 0 ; i < dir_num ; i++)
	{
		if(NULL != list[i].name)
		{
			free(list[i].name);
			list[i].name = NULL;
		}
	}
	free(list);
	list = NULL;
	
	return;
}

//获取SD卡根目录下的录像文件存放路径的个数
int _webcmd_getDirCnt(const char *dirPath, int *dir_num)
{
	DIR	*pDir;
	struct dirent *pDirent = NULL;
	int year,mon,day;
	int dir_num_tmp = 0;

	//入参合法性校验，不允许为空
	if(NULL == dirPath || NULL == dir_num)
	{
		return -1;
	}

	//打开要遍历的目录/progs/rec/00/
	pDir = opendir(dirPath);
	if(NULL == pDir)
	{
		return -1;
	}

	//遍历目标目录下的文件夹
	while((pDirent = readdir(pDir)) != NULL)
	{
		//找到类似20150813命名格式的文件夹
		if(8 == strlen(pDirent->d_name) && (sscanf(pDirent->d_name, "%04d%02d%02d", &year, &mon, &day) == 3))
		{
			dir_num_tmp++;
		}
	}
	closedir(pDir);
	*dir_num = dir_num_tmp;
	
	return 0;
}

//获取路径下.mp4录像文件个数
int _webcmd_getEventCnt(char *dirPath, int *event_num)
{
	DIR	*pDir;
	struct dirent *pDirent = NULL;
	int event_num_tmp = 0;

	//入参合法性校验，不允许为空
	if(NULL == dirPath || NULL == event_num)
	{
		return -1;
	}

	//打开目标路径，类似/progs/rec/00/20150820/
	pDir = opendir(dirPath);
	if(NULL == pDir)
	{
		return -1;
	}

	//遍历目标路径下的所有.mp4文件并计数
	while((pDirent = readdir(pDir)) != NULL)
	{
		if(NULL != strstr(pDirent->d_name, ".mp4"))
		{
			event_num_tmp++;
		}
	}
	closedir(pDir);
	*event_num = event_num_tmp;
	
	return 0;
}

//cmd=webrecord&action=event_get的操作函数，获取SD卡内所有的录像文件列表
int _webcmd_getEventListFromSD(const char *dirPath)
{
	DIR	*pDir, *pDir_event;
	struct dirent *pDirent = NULL;
	struct dirent *pDirent_event	 = NULL;
	char tmp_dir[20] = {0};
	int recCount = 0;
	int eventCount = 0;
	int year,mon,day;
	
	prec_file_list_t list = NULL;
	prec_file_t eventList = NULL;
	int dir_num = 0;
	int event_num = 0;
	int dir_tmp_count = 0;
	int event_tmp_count = 0;
	char *tmp,*out;

	//入参合法性校验，不允许为空
	if(NULL == dirPath)
	{
		printf("input params invalid .\n");
		weber_set_result("input params invalid .");
		return -1;
	}
	
	//打开/progs/rec/00/目录，即SD卡根目录
	pDir = opendir(dirPath);
	if(NULL == pDir)
	{
		printf("opendir %s fail.\n",dirPath);
		weber_set_result("opendir fail.");
		return -1;
	}

	//获取录像文件夹的个数
	if(0 != _webcmd_getDirCnt(dirPath,&dir_num))
	{
		printf("get dir num fail , %s.\n",dirPath);
		weber_set_result("get dir num fail.");
		closedir(pDir);
		return -1;
	}

	//根据目录个数申请内存
	list = (prec_file_list_t)malloc(dir_num * sizeof(rec_file_list_t));
	if(NULL == list)
	{
		printf("malloc for list fail.\n");
		weber_set_result("internal error.");
		closedir(pDir);
		return -1;
	}
	memset(list,0,dir_num * sizeof(rec_file_list_t));
	dir_tmp_count = 0;

	//遍历SD卡根目录
	while((pDirent = readdir(pDir)) != NULL)
	{
		//找到类似20150813命名格式的文件夹
		if(8 == strlen(pDirent->d_name) && (sscanf(pDirent->d_name, "%04d%02d%02d", &year, &mon, &day) == 3))
		{
			sprintf(tmp_dir,"%s%s/",dirPath,pDirent->d_name);

			if(0 != access(tmp_dir,F_OK))
			{
				printf("cannot access path : %s.\n",tmp_dir);
				weber_set_result("cannot access path.");
				_webcmd_freeListIfNecessary(list,dir_num);
				closedir(pDir);
				return -1;
			}

			//进入固定格式命名的文件夹
			pDir_event = opendir(tmp_dir);
			if(NULL == pDir_event)
			{
				printf("opendir %s fail.\n",tmp_dir);
				weber_set_result("opendir fail.");
				_webcmd_freeListIfNecessary(list,dir_num);
				closedir(pDir);
				return -1;
			}

			strcpy(list[dir_tmp_count].date,pDirent->d_name);

			//获取目的目录下的.mp4录像文件的个数
			if(0 != _webcmd_getEventCnt(tmp_dir,&event_num))
			{
				printf("get event num fail , %s.\n",tmp_dir);
				weber_set_result("get event num fail.");
				_webcmd_freeListIfNecessary(list,dir_num);
				closedir(pDir_event);
				closedir(pDir);
				return -1;
			}

			list[dir_tmp_count].cnt = event_num;

			//根据录像文件个数申请临时指针的内存
			eventList = (prec_file_t)malloc(event_num * sizeof(rec_file_t));
			if(NULL == eventList)
			{
				printf("malloc for eventList fail.\n");
				weber_set_result("internal error.");
				_webcmd_freeListIfNecessary(list,dir_num);
				closedir(pDir_event);
				closedir(pDir);
				return -1;
			}
			memset(eventList,0,event_num * sizeof(rec_file_t));
			event_tmp_count = 0;

			//遍历目录内容，找到.mp4的文件并追加到eventList中
			while((pDirent_event = readdir(pDir_event)) != NULL)
			{
				if(NULL != strstr(pDirent_event->d_name, ".mp4"))
				{
					strcpy(eventList[event_tmp_count].name,pDirent_event->d_name);
					event_tmp_count++;
				}
			}

			//将临时指针存放的录像文件名称转换为json格式并赋给name
			tmp = (char *)malloc(MAX_EVENT_LEN * event_num);
			if(NULL == tmp)
			{
				printf("malloc for tmp return error\n");
				weber_set_result("internal error");
				_webcmd_freeListIfNecessary(list,dir_num);
				free(eventList);
				closedir(pDir_event);
				closedir(pDir);
				return -1;
			}
			memset(tmp,0,MAX_EVENT_LEN * event_num);
			int i = 0;
			for(i = 0 ; i < event_num ; i++)
			{
				if(i == (event_num - 1))
				{
					strcat(tmp,eventList[i].name);
					break;
				}
				strcat(tmp,eventList[i].name);
				strcat(tmp,",");
			}
			
			list[dir_tmp_count].name = tmp;
			dir_tmp_count++;
			
			free(eventList);
			closedir(pDir_event);
		}
	}
	closedir(pDir);

	//将整理好的整个的list内容转换为json格式并输出给用户
	out = cjson_object_array2string(json_rec_file_list,list,dir_num);
	if(NULL != out)
	{
		weber_set_result(out);
		free(out);
	}
	else
	{
		printf("out json return error\n");
		weber_set_result("json error");
	}
	
	_webcmd_freeListIfNecessary(list,dir_num);
	return 0;
}

static void _webcmd_save()
{
	WriteConfigInfo();
}

typedef struct __MDPW
{
	char		acID[SIZE_ID];	///< ID注册后不可更改，但可以删除
	char		acOldPW[SIZE_PW];
	char		acNewPW[SIZE_PW];	///< 密码，可以更改
}MDPW, *PMDPW;

static json_kinfo_t json_mdpw[] =
{
	MAKE_KEY_INFO(MDPW, KEY_TYPE_STRING, acID, NULL),
	MAKE_KEY_INFO(MDPW, KEY_TYPE_STRING, acOldPW, NULL),
	MAKE_KEY_INFO(MDPW, KEY_TYPE_STRING, acNewPW, NULL),

	MAKE_END_INFO()
};

static json_kinfo_t json_account[] =
{
	MAKE_KEY_INFO(ACCOUNT, KEY_TYPE_U32, nIndex, NULL),
	MAKE_KEY_INFO(ACCOUNT, KEY_TYPE_STRING, acID, NULL),
	MAKE_KEY_INFO(ACCOUNT, KEY_TYPE_STRING, acPW, NULL),
	MAKE_KEY_INFO(ACCOUNT, KEY_TYPE_STRING, acDescript, NULL),
	MAKE_KEY_INFO(ACCOUNT, KEY_TYPE_U32, nPower, NULL),

	MAKE_END_INFO()
};

#define webaccount_GENERAL	"account operation cmd"
#define webaccount_DETAIL	\
	"\nUsage: webaccount OPERATION [PARAM]\n"\
	"Operations:\n"\
	"  list\r\t\tList all account\n"\
	"  add\r\t\tAdd an account with PARAM\n"\
	"  modify\r\t\tModify an account with PARAM\n"\
	"  del\r\t\tDelete an account with PARAM\n"\
	"  check\r\t\tCheck whether account passwd is right\n"\
	"\n"\
	"PARAM:\n"\
	"  param is setted when OPTION is add, modify, del and check\n"\
	"Example:\n"\
	"  webaccount add '{\"acID\": \"gary\",\"acPW\": \"123\",\"acDescript\":\"test\",\"nPower\":17}'\n"\
	"\n"

int webcmd_account(int argc, char *argv[])
{
	ACCOUNT act, *mp;
	MDPW mdpw;
	int ret;
	char *result;
	WEBRESULT rettest;

	//初始化一下act，否则nPower会被初始化为30
	memset(&act,0,sizeof(ACCOUNT));

	//加入用户鉴权后，参数个数变多
	//if (argc >= 3)
	if (argc >= ((CheckPower == 1)?5:3)
		&& strcmp(argv[1], "list") != 0
		&& strcmp(argv[1], "count") != 0)  //count操作不需要进行json转换，count的argv[2]是一个数字或者没有argv[2]
	{
		mp = cjson_string2object(json_account,argv[2],&act);
		if (mp == NULL)
		{
			//rettest.status="abc";
			//rettest.data="abcd";
			//weber_format_result(&rettest);
			weber_set_result("param error");
			return -1;
		}
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[1],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}

	if (strcmp(argv[1], "list") == 0)
	{
		ACCOUNT *act;
		int cnt = maccount_get_cnt();
		int i;
		char *out;

		act = malloc(sizeof(ACCOUNT)*cnt);
		jv_assert(act, return -1);
		for (i=0; i<cnt; i++)
		{
			act[i] = *maccount_get(i);
			printf("acdName: %s\n",act[i].acID);
		}
		out = cjson_object_array2string(json_account,act,cnt);
		jv_assert(out, {free(act); return -1;});
		weber_set_result(out);
		free(out);
		free(act);
		return 0;
	}

	//if (argc != 3 || strlen(act.acID) == 0)
	if ((argc != ((CheckPower == 1)?5:3) || strlen(act.acID) == 0)
		&& strcmp(argv[1], "count") != 0)  //count操作不需要做校验
	{
		weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
		return -1;
	}
	if (strcmp(argv[1], "add") == 0)
	{
		//设置nPower的缺省值为2，即普通用户
		if(0 == act.nPower)
		{
			act.nPower = POWER_USER;
		}
		//nPower非缺省值的情况下，只支持1,2,4三种权限，其他的都返回失败
		else if(POWER_ADMIN != act.nPower && POWER_GUEST != act.nPower && POWER_USER != act.nPower)
		{
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			printf("illegal power value %d \n",act.nPower);
			return -1;
		}
		ret = maccount_add(&act);
	}
	else if(strcmp(argv[1], "modify") == 0)
	{
		cjson_string2object(json_mdpw,argv[2],&mdpw);
		ret = maccount_modifypw(mdpw.acID,mdpw.acOldPW,mdpw.acNewPW);
	}
	else if(strcmp(argv[1], "del") == 0)
	{
		ret = maccount_remove(&act);
	}
	else if(strcmp(argv[1], "check") == 0)
	{
//		char name_gb2312[20];
//		utl_iconv_utf8togb2312(act.acID,name_gb2312,20);
		ret = maccount_check_power_ex(act.acID, act.acPW);
		//printf("_________________%x\n",ret);
		if (ret == 0 || ret == ERR_PASSWD_EX)
			ret = ERR_PASSWD_EX;
		else if(ERR_NOTEXIST_EX == ret)
			ret = ERR_NOTEXIST_EX;
		else
			ret =0;
	}
	else if(strcmp(argv[1], "count") == 0)
	{
		SPConType_e conType = SP_CON_ALL;
		char strRlt[50];
		char str[SP_CON_OTHER+1][20] = {"con_all",
										"con_jovision",
										"con_rtsp",
										"con_gb28181",
										"con_psia",
										"con_other"};
		if(argc == ((CheckPower == 1)?4:2))
		{
			conType = SP_CON_ALL;  //argv[2]缺省情况下，默认查询全部类型的用户
		}
		else
		{
			if(atoi(argv[2]) < SP_CON_ALL || atoi(argv[2]) > SP_CON_OTHER)
			{
				conType = SP_CON_ALL;  //参数不在有效范围内时，查询所有类型的用户
			}
			else
			{
				conType = (SPConType_e) atoi(argv[2]);
			}
		}
		ret = sp_connect_get_cnt(conType);

		memset(strRlt,'\0',50);
		sprintf(strRlt,"Type %s connected user count:%d",str[conType],ret);
		weber_set_result(strRlt);
		return 0;
	}
	switch(ret)
	{
	default:
		result = "{\"status\":\"param error\",\"data\":\"\"}";
		break;
	case 0:
		result = "{\"status\":\"ok\",\"data\":\"\"}";
		break;
	case ERR_EXISTED:
		result = "{\"status\":\"account existed\",\"data\":\"\"}";
		break;
	case ERR_LIMITED:
		result = "{\"status\":\"too many account\",\"data\":\"\"}";
		break;
	case ERR_NOTEXIST_EX:
		result = "{\"status\":\"account not exist\",\"data\":\"\"}";
		printf(result);
		break;
	case ERR_PASSWD_EX:
		result = "{\"status\":\"error passwd\",\"data\":\"\"}";
		break;
	case ERR_PERMISION_DENIED:
		result = "{\"status\":\"without permission\",\"data\":\"\"}";
		break;
	}
	weber_set_result(result);
	_webcmd_save();
	return 0;
}

//begin added by zhouwq 20150703 {
static json_kinfo_t json_alarmTime[] =
{
	//数据结构从AlarmTime改为SPAlarmTime以调用sp接口
	MAKE_KEY_INFO(SPAlarmTime, KEY_TYPE_STRING, 	tStart, NULL),	// 安全防护开始时间(格式: hh:mm:ss)
	MAKE_KEY_INFO(SPAlarmTime, KEY_TYPE_STRING, 	tEnd, 	NULL),	// 安全防护结束时间(格式: hh:mm:ss)

	MAKE_END_INFO()
};
//} end added

static json_kinfo_t json_schedule[] =
{
	MAKE_KEY_INFO(MSchedule, KEY_TYPE_U32, bEnable, 		NULL),
	MAKE_KEY_INFO(MSchedule, KEY_TYPE_U32, Schedule_time_H, NULL),
	MAKE_KEY_INFO(MSchedule, KEY_TYPE_U32, Schedule_time_M, NULL),
	MAKE_KEY_INFO(MSchedule, KEY_TYPE_U32, num, 			NULL),
	MAKE_KEY_INFO(MSchedule, KEY_TYPE_U32, Interval,		NULL),

	MAKE_END_INFO()
};

static json_kinfo_t json_alarm[] =
{
	MAKE_KEY_INFO(ALARMSET, KEY_TYPE_U32, delay, NULL),  //数据结构从ALARMSET改为SPAlarmSet_t以调用sp接口
	MAKE_KEY_INFO(ALARMSET, KEY_TYPE_STRING, sender, NULL),
	MAKE_KEY_INFO(ALARMSET, KEY_TYPE_STRING, server, NULL),
	MAKE_KEY_INFO(ALARMSET, KEY_TYPE_STRING, username, NULL),
	MAKE_KEY_INFO(ALARMSET, KEY_TYPE_STRING, passwd, NULL),
	MAKE_KEY_INFO(ALARMSET, KEY_TYPE_STRING, receiver0, NULL),
	MAKE_KEY_INFO(ALARMSET, KEY_TYPE_STRING, receiver1, NULL),
	MAKE_KEY_INFO(ALARMSET, KEY_TYPE_STRING, receiver2, NULL),
	MAKE_KEY_INFO(ALARMSET, KEY_TYPE_STRING, receiver3, NULL),
	MAKE_ARRAY_INFO(ALARMSET, KEY_TYPE_ARRAY,	m_Schedule, json_schedule, 5, KEY_TYPE_OBJECT),
	MAKE_KEY_INFO(ALARMSET, KEY_TYPE_U32,		port, 	NULL),
	MAKE_KEY_INFO(ALARMSET, KEY_TYPE_STRING, 	crypto, NULL),
	MAKE_KEY_INFO(ALARMSET, KEY_TYPE_U32, 		bEnable, NULL),
	MAKE_ARRAY_INFO(ALARMSET, KEY_TYPE_ARRAY, 	alarmTime, json_alarmTime, MAX_ALATM_TIME_NUM, KEY_TYPE_OBJECT),
	MAKE_KEY_INFO(ALARMSET, KEY_TYPE_STRING,	vmsServerIp, NULL),
	MAKE_KEY_INFO(ALARMSET, KEY_TYPE_U32, 		vmsServerPort, NULL),
	MAKE_KEY_INFO(ALARMSET, KEY_TYPE_U32, 		bAlarmSoundEnable, NULL),
	MAKE_KEY_INFO(ALARMSET, KEY_TYPE_U32, 		bAlarmLightEnable, NULL),

	MAKE_END_INFO()
};

#define webalarm_GENERAL	"alarm operation command"
#define webalarm_DETAIL	\
	"\nUsage: webalarm OPERATION [PARAM]\n"\
	"Operations:\n"\
	"  list\r\t\tList the alarm info\n"\
	"  set\r\t\tSet the alarm info with PARAM\n"\
	"\n"\
	"PARAM:\n"\
	"  param is setted when OPTION is set\n"\
	"Example:\n"\
	"  webalarm set '{\"delay\":10,\"sender\":\"ipcmail@163.com\",\"server\":\"smtp.163.com\",\"username\":\"ipcmail\",\"passwd\":\"ipcam71a\",\"receiver0\":\"lfx@jovision.com\",\"receiver1\":\"(null)\",\"receiver2\":\"(null)\",\"receiver3\":\"(null)\"}'\n"\
	"\n"

int webcmd_alarm(int argc, char *argv[])
{
	ALARMSET alarm, *ap, alarm_bak;
	//SPAlarmSet_t alarm, *ap;
	char *out;
	int i = 0, time_tmp = 0;

	//加入用户鉴权后，参数个数变多
	//if (argc < 2)
	if (argc < ((CheckPower == 1)?4:2))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("param error");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[1],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}

	//接口更换为sp接口
	malarm_get_param(&alarm);
	//memset(&alarm,0,sizeof(SPAlarmSet_t));
	//sp_alarm_get_param(&alarm);
	if (strcmp(argv[1], "list") == 0)
	{
		out = cjson_object2string(json_alarm,&alarm);
		if (out != NULL)
		{
			weber_set_result(out);
			free(out);
			return 0;
		}
		else
		{
			weber_set_result("unknown error");
			return -1;
		}
	}
	else if (strcmp(argv[1], "set") == 0)
	{
		//加入用户鉴权后，参数多两个
		//if (argc != 3)
		if (argc != ((CheckPower == 1)?5:3))
		{
			printf("the number of params isn't right , argc=%d\n",argc);
			weber_set_result("param error");
			return -1;
		}
		ap = cjson_string2object(json_alarm, argv[2], &alarm);
		if (ap == NULL)
		{
			weber_set_result("param error");
			return -1;
		}

		malarm_get_param(&alarm_bak);
		for(i=0;i<5;i++)
		{
			if(memcmp(&alarm.m_Schedule[i],&alarm_bak.m_Schedule[i],sizeof(alarm_bak.m_Schedule[i])))
			{
			    if(alarm.m_Schedule[i].bEnable == 1)
			    {
			    	time_tmp = alarm.m_Schedule[i].Schedule_time_H * 60 + alarm.m_Schedule[i].Schedule_time_M;
			   		utl_schedule_Enable(schedule_id[i],time_tmp);			
			    }
				else
				{
					utl_schedule_disable(schedule_id[i]);
				}
			}
		}
		
		weber_set_result("ok");
		malarm_set_param(&alarm);
		_webcmd_save();
		//sp_alarm_set_param(&alarm);
	}
	else if (strcmp(argv[1], "triggeralarmin") == 0)
	{
		if (ma_timer == -1)
			ma_timer = utl_timer_create("malarmIn", 0, _malarmin_process, (void*)3);
		else
			utl_timer_reset(ma_timer, 0, _malarmin_process, (void*)3);
		weber_set_result("ok");
	}
	return 0;
}


static json_kinfo_t json_rect[] = {
	//数据结构从RECT改为SPRect_t以调用sp接口
	MAKE_KEY_INFO(SPRect_t, KEY_TYPE_U32, x, NULL),
	MAKE_KEY_INFO(SPRect_t, KEY_TYPE_U32, y, NULL),
	MAKE_KEY_INFO(SPRect_t, KEY_TYPE_U32, w, NULL),
	MAKE_KEY_INFO(SPRect_t, KEY_TYPE_U32, h, NULL),
	MAKE_END_INFO()
};

static json_kinfo_t json_mdetect[] =
{
	//数据结构从MD改为SPMdetect_t以调用sp接口
	MAKE_KEY_INFO(SPMdetect_t, KEY_TYPE_U32, bEnable		, NULL),
	MAKE_KEY_INFO(SPMdetect_t, KEY_TYPE_U32, bEnableRecord, NULL), //是否开启报警录像
	MAKE_KEY_INFO(SPMdetect_t, KEY_TYPE_U32, nSensitivity, NULL),
	MAKE_KEY_INFO(SPMdetect_t, KEY_TYPE_U32, nRectNum	, NULL),
	MAKE_ARRAY_INFO(SPMdetect_t, KEY_TYPE_ARRAY, stRect	, json_rect, 4, KEY_TYPE_OBJECT),
	MAKE_KEY_INFO(SPMdetect_t, KEY_TYPE_U32, nDelay		, NULL),
	MAKE_KEY_INFO(SPMdetect_t, KEY_TYPE_U32, bOutClient	, NULL),
	MAKE_KEY_INFO(SPMdetect_t, KEY_TYPE_U32, bOutEMail	, NULL),

	MAKE_END_INFO()
};

#define webmdetect_GENERAL	"motion detect operation command"
#define webmdetect_DETAIL	\
	"\nUsage: webmdetect CHANNELID OPERATION [PARAM]\n"\
	"CHANNELID: \n"\
	"  The ID such as 1,2,3,4... For IPC, It is 1 always\n"\
	"\n"\
	"Operations:\n"\
	"  list\r\t\tList the mdetect info\n"\
	"  set\r\t\tSet the mdetect info with PARAM\n"\
	"\n"\
	"PARAM:\n"\
	"  PARAM is setted when OPTION is \"set\"\n"\
	"  You can use cmd: 'webmdetect list' to see the format of PARAM\n"\
	"Example:\n"\
	"  webmdetect 1 set '{\"bEnable\":0,\"nSensitivity\":50,\"nThreshold\":15,\"nRectNum\":0,\"stRect\":[{\"x\":0,\"y\":0,\"w\":0,\"h\":0},{\"x\":0,\"y\":0,\"w\":0,\"h\":0},{\"x\":0,\"y\":0,\"w\":0,\"h\":0},{\"x\":0,\"y\":0,\"w\":0,\"h\":0}],\"nDelay\":10,\"nStart\":0,\"bOutClient\":0,\"bOutEMail\":0}'\n"\
	"\n"

int webcmd_mdetect(int argc, char *argv[])
{
	//更换sp接口
	//MD md, *mp;
	SPMdetect_t md, *mp;
	char *out;
	int channelid;

	//加入用户鉴权后，参数个数变多
	//if (argc < 3)
	if (argc < ((CheckPower == 1)?5:3))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("param error");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[2],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}

	channelid = atoi(argv[1])-1;
	//更换sp接口
	//mdetect_get_param(channelid, &md);
	memset(&md,0,sizeof(SPMdetect_t));
	sp_mdetect_get_param(channelid,&md);
	if (strcmp(argv[2], "list") == 0)
	{
		out = cjson_object2string(json_mdetect,&md);
		if (out != NULL)
		{
			weber_set_result(out);
			free(out);
			return 0;
		}
		else
		{
			weber_set_result("unknown error");
			return -1;
		}
	}
	else if (strcmp(argv[2], "set") == 0)
	{
		//加入用户鉴权操作后，参数多了两个
		//if (argc != 4)
		if (argc != ((CheckPower == 1)?6:4))	
		{
			printf("the number of params isn't right , argc=%d\n",argc);
			weber_set_result("param error");
			return -1;
		}
		mp = cjson_string2object(json_mdetect, argv[3], &md);
		if (mp == NULL)
		{
			weber_set_result("param error");
			return -1;
		}
		weber_set_result("ok");
		//更换sp接口
		//mdetect_set_param(channelid, &md);
		//mdetect_flush(channelid);
		//_webcmd_save();
		sp_mdetect_set_param(channelid,&md);
	}
	else if (strcmp(argv[2], "balarm") == 0)
	{
		BOOL rlt = FALSE;
		rlt = sp_mdetect_balarming(channelid);

		if(rlt)
			weber_set_result("balarming : true");
		else
			weber_set_result("balarming : false");
	}
	//LSK518客户增加一个触发移动侦测报警的接口
	else if(strcmp(argv[2], "detectstart") == 0)
	{
		if (md_timer[channelid] == -1)
			md_timer[channelid] = utl_timer_create("mdetect", 0, _mdetect_timer_callback, NULL);
		else
			utl_timer_reset(md_timer[channelid], 0, _mdetect_timer_callback, NULL);
		weber_set_result("ok");
	}
	return 0;
}

#define weblog_GENERAL	"log operation command"
#define weblog_DETAIL	\
	"\nUsage: weblog list\n"\
	"Operations:\n"\
	"  list\r\t\tList the log info\n"\
	"\n"

int webcmd_log(int argc, char *argv[])
{
	weber_set_result("not support now");
	return 0;
}

static json_kinfo_t json_privacy[] =
{
	//接口从REGION改为SPRegion_t
	MAKE_KEY_INFO(SPRegion_t, KEY_TYPE_U32, bEnable		, NULL),
	MAKE_ARRAY_INFO(SPRegion_t, KEY_TYPE_ARRAY, stRect	, json_rect, MAX_PYRGN_NUM, KEY_TYPE_OBJECT),

	MAKE_END_INFO()
};

#define webprivacy_GENERAL	"privacy operation command"
#define webprivacy_DETAIL	\
	"\nUsage: webprivacy CHANNELID OPERATION [PARAM]\n"\
	"CHANNELID: \n"\
	"  The ID such as 1,2,3,4... For IPC, It is 1 always\n"\
	"\n"\
	"Operations:\n"\
	"  list\r\t\tList the privacy info\n"\
	"  set\r\t\tSet the privacy info with PARAM\n"\
	"\n"\
	"PARAM:\n"\
	"  PARAM is setted when OPTION is \"set\"\n"\
	"  You can use cmd: 'webprivacy list' to see the format of PARAM\n"\
	"Example:\n"\
	"   webprivacy 1 set '{\"bEnable\":0,\"stRect\":[{\"x\":0,\"y\":0,\"w\":0,\"h\":0},{\"x\":0,\"y\":0,\"w\":0,\"h\":0},{\"x\":0,\"y\":0,\"w\":0,\"h\":0},{\"x\":0,\"y\":0,\"w\":0,\"h\":0}]}'\n"\
	"\n"

int webcmd_privacy(int argc, char *argv[])
{
	//REGION region, *mp;
	SPRegion_t region, *mp;
	char *out;
	int channelid;

	//加入用户鉴权后，参数个数变多
	//if (argc < 3)
	if (argc < ((CheckPower == 1)?5:3))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("param error");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[2],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}

	channelid = atoi(argv[1])-1;
	printf("region offset: %d\n", NAME_OFFSET(REGION, stRect));
	//mprivacy_get_param(channelid, &region);
	memset(&region,0,sizeof(SPRegion_t));
	sp_privacy_get_param(channelid,&region);
	if (strcmp(argv[2], "list") == 0)
	{
		printf("%d ===> region.stRect[3].w : %d\n", __LINE__, region.stRect[3].w);
		out = cjson_object2string(json_privacy,&region);
		if (out != NULL)
		{
			weber_set_result(out);
			free(out);
			return 0;
		}
		else
		{
			weber_set_result("unknown error");
			return -1;
		}
	}
	else if (strcmp(argv[2], "set") == 0)
	{
		//加入用户鉴权操作后，参数多了两个
		//if (argc != 4)
		if (argc != ((CheckPower == 1)?6:4))	
		{
			printf("the number of params isn't right , argc=%d\n",argc);
			weber_set_result("param error");
			return -1;
		}
		mp = cjson_string2object(json_privacy, argv[3], &region);
		if (mp == NULL)
		{
			weber_set_result("param error");
			return -1;
		}
		weber_set_result("ok");
		//mprivacy_set_param(channelid, &region);
		//mprivacy_flush(channelid);
		//_webcmd_save();
		sp_privacy_set_param(channelid,&region);
	}
	return 0;
}


static json_kinfo_t json_record[] =
{
	MAKE_KEY_INFO(mrecord_attr_t, KEY_TYPE_U32, bEnable		, NULL),
	MAKE_KEY_INFO(mrecord_attr_t, KEY_TYPE_U32, file_length	, NULL),
	MAKE_KEY_INFO(mrecord_attr_t, KEY_TYPE_U32, timing_enable, NULL),
	//begin added by zhouwq 20150706 {
	MAKE_KEY_INFO(mrecord_attr_t, KEY_TYPE_U32, discon_enable, NULL), ///< 是否启用无连接录像
	MAKE_KEY_INFO(mrecord_attr_t, KEY_TYPE_U32, alarm_enable, NULL), ///< 是否启用报警录像
	//}end added
	MAKE_KEY_INFO(mrecord_attr_t, KEY_TYPE_U32, timing_start	, NULL),
	MAKE_KEY_INFO(mrecord_attr_t, KEY_TYPE_U32, timing_stop	, NULL),
	//begin added by zhouwq 20150706 {
	MAKE_KEY_INFO(mrecord_attr_t, KEY_TYPE_U32, disconnected, NULL), ///< 无连接时录制
	//}end added
	MAKE_KEY_INFO(mrecord_attr_t, KEY_TYPE_U32, detecting	, NULL),
	MAKE_KEY_INFO(mrecord_attr_t, KEY_TYPE_U32, alarming	, NULL),
	MAKE_KEY_INFO(mrecord_attr_t, KEY_TYPE_U32, alarm_pre_record	, NULL),
	MAKE_KEY_INFO(mrecord_attr_t, KEY_TYPE_U32, alarm_duration	, NULL),

	MAKE_END_INFO()
};

#define webrecord_GENERAL	"record operation command"
#define webrecord_DETAIL	\
	"\nUsage: webrecord CHANNELID OPERATION [PARAM]\n"\
	"CHANNELID: \n"\
	"  The ID such as 1,2,3,4... For IPC, It is 1 always\n"\
	"\n"\
	"Operations:\n"\
	"  list\r\t\tList the record info\n"\
	"  set\r\t\tSet the record info with PARAM\n"\
	"\n"\
	"PARAM:\n"\
	"  PARAM is setted when OPTION is \"set\"\n"\
	"  You can use cmd: 'webrecord list' to see the format of PARAM\n"\
	"Example:\n"\
	"  webrecord 1 set '{\"bEnable\":0,\"file_length\":1800,\"timing_enable\":0,\"timing_start\":0,\"timing_stop\":0,\"detecting\":0,\"alarming\":0,\"alarm_pre_record\":0,\"alarm_duration\":0}'\n"\
	"\n"
int webcmd_record(int argc, char *argv[])
{
	mrecord_attr_t record, *mp;
	char *out;
	int channelid;

	//加入用户鉴权后，参数个数变多
	//if (argc < 3)
	if (argc < ((CheckPower == 1)?5:3))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("param error");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[2],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}

	channelid = atoi(argv[1])-1;
	mrecord_get_param(channelid, &record);
	if (strcmp(argv[2], "list") == 0)
	{
		out = cjson_object2string(json_record,&record);
		if (out != NULL)
		{
			weber_set_result(out);
			free(out);
			return 0;
		}
		else
		{
			weber_set_result("unknown error");
			return -1;
		}
	}
	else if (strcmp(argv[2], "set") == 0)
	{
		//加入用户鉴权后，参数多了两个
		//if (argc != 4)
		if (argc != ((CheckPower == 1)?6:4))	
		{
			printf("the number of params isn't right , argc=%d\n",argc);
			weber_set_result("param error");
			return -1;
		}
		mp = cjson_string2object(json_record, argv[3], &record);
		if (mp == NULL)
		{
			weber_set_result("param error");
			return -1;
		}
		weber_set_result("ok");
		mrecord_set_param(channelid, &record);
		mrecord_flush(channelid);
		_webcmd_save();
	}
	else if(strcmp(argv[2], "event_get") == 0)
	{
		if (0 != access(mstorage_get_cur_recpath(NULL, 0), F_OK))  //查看TF卡是否挂载成功
		{
			printf("cannot access SD card .\n");
			weber_set_result("The status of SD card isn't normal . can't access dir .");
		}
		else
		{
			int rlt = _webcmd_getEventListFromSD(mstorage_get_cur_recpath(NULL, 0));
			if(rlt != 0)
			{
				printf("_webcmd_getEventListFromSD return error .\n");
			}
		}
	}
	return 0;
}

static json_kinfo_t json_storage[] =
{
	MAKE_KEY_INFO(SPStorage_t, KEY_TYPE_U32,nSize		 	, NULL),
	MAKE_KEY_INFO(SPStorage_t, KEY_TYPE_U32,nCylinder	 	, NULL),
	MAKE_KEY_INFO(SPStorage_t, KEY_TYPE_U32,nPartSize	, NULL),
	MAKE_KEY_INFO(SPStorage_t, KEY_TYPE_U32,nPartition	, NULL),
	MAKE_KEY_INFO(SPStorage_t, KEY_TYPE_U32,nEntryCount	, NULL),
	MAKE_KEY_INFO(SPStorage_t, KEY_TYPE_U32,nStatus			, NULL),
	MAKE_KEY_INFO(SPStorage_t, KEY_TYPE_U32,nCurPart		, NULL),
	MAKE_KEY_INFO(SPStorage_t, KEY_TYPE_U32,nocard			, NULL),
	MAKE_ARRAY_INFO(SPStorage_t, KEY_TYPE_ARRAY,nPartSpace	, NULL, PARTS_PER_DEV, KEY_TYPE_U32),
	MAKE_ARRAY_INFO(SPStorage_t, KEY_TYPE_ARRAY,nFreeSpace	, NULL, PARTS_PER_DEV, KEY_TYPE_U32),
	//begin added by zhouwq 20150706 {
	MAKE_KEY_INFO(SPStorage_t, KEY_TYPE_U32,mounted			, NULL),
	//}end added

	MAKE_END_INFO()
};

#define webstorage_GENERAL	"storage operation command"
#define webstorage_DETAIL	\
	"\nUsage: storage OPERATION\n"\
	"Operations:\n"\
	"  list\r\t\tList the storage info\n"\
	"  format\r\t\tFormat the storage \n"\
	"\n"

int webcmd_storage(int argc, char *argv[])
{
	//STORAGE storage, *mp;
	SPStorage_t storage, *mp;
	char *out;

	//加入用户鉴权后，参数个数变化
	//if (argc != 2)
	if (argc != ((CheckPower == 1)?4:2))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("param error");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[1],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}

	memset(&storage,0,sizeof(SPStorage_t));
	sp_storage_get_info(&storage);

	if (strcmp(argv[1], "list") == 0)
	{
		//mstorage_get_info(&storage);
		//storage.nPartSpace[0] = 100;
		//storage.nPartSpace[1] = 200;
		//storage.nPartSpace[2] = 300;
		
		out = cjson_object2string(json_storage,&storage);
		if (out != NULL)
		{
			weber_set_result(out);
			free(out);
			return 0;
		}
		else
		{
			weber_set_result("unknown error");
			return -1;
		}
	}
	else if (strcmp(argv[1], "format") == 0)
	{
		//mstorage_get_info(&storage);
		if(STG_USING != storage.nStatus && STG_NONE != storage.nStatus)
		{
			//if(0 == mstorage_format(0))
			//{
			//}
			//if(0 != mstorage_mount())
			if(0 != sp_storage_format(0))
			{
				weber_set_result("storage mount error");
			}
			else
				weber_set_result("ok");
		}
		else
			weber_set_result("device is busy");
	}
	return 0;
}

static json_kinfo_t json_stream[] =
{
	MAKE_KEY_INFO(SPStreamAttr_t, KEY_TYPE_U32, bEnable		, NULL),
	//begin added by zhouwq 20150706 {
	MAKE_KEY_INFO(SPStreamAttr_t, KEY_TYPE_U32, bAudioEn	, NULL), //是否带有音频数据
	MAKE_KEY_INFO(SPStreamAttr_t, KEY_TYPE_U32, viWidth		, NULL), ///< 输入的宽度
	MAKE_KEY_INFO(SPStreamAttr_t, KEY_TYPE_U32, viHeight	, NULL), ///< 输入的高度
	//}end added
	MAKE_KEY_INFO(SPStreamAttr_t, KEY_TYPE_U32, width	 	, NULL),
	MAKE_KEY_INFO(SPStreamAttr_t, KEY_TYPE_U32, height	 	, NULL),
	MAKE_KEY_INFO(SPStreamAttr_t, KEY_TYPE_U32, framerate 	, NULL),
	MAKE_KEY_INFO(SPStreamAttr_t, KEY_TYPE_U32, bitrate	 	, NULL),
	MAKE_KEY_INFO(SPStreamAttr_t, KEY_TYPE_U32, ngop_s	 	, NULL),
	MAKE_KEY_INFO(SPStreamAttr_t, KEY_TYPE_U32, rcMode	 	, NULL),
	MAKE_KEY_INFO(SPStreamAttr_t, KEY_TYPE_U32, encLevel	, NULL),
	MAKE_KEY_INFO(SPStreamAttr_t, KEY_TYPE_U32, quality		, NULL),
	MAKE_KEY_INFO(SPStreamAttr_t, KEY_TYPE_U32, minQP	 	, NULL),
	MAKE_KEY_INFO(SPStreamAttr_t, KEY_TYPE_U32, maxQP	 	, NULL),
	//MAKE_KEY_INFO(mstream_attr_t, KEY_TYPE_U32, bRectStretch, NULL), //是拉伸，还是裁剪。为真时表示拉伸
	//MAKE_KEY_INFO(mstream_attr_t, KEY_TYPE_U32, vencType	, NULL), // 视频编码协议类型

	MAKE_END_INFO()
};

static json_kinfo_t json_stream_resolution[] =
{
	MAKE_KEY_INFO(SPRes_t, KEY_TYPE_U32, w		, NULL),
	MAKE_KEY_INFO(SPRes_t, KEY_TYPE_U32, h	 	, NULL),

	MAKE_END_INFO()
};

static json_kinfo_t json_stream_ability[] =
{
	//begin added by zhouwq 20150706 {
	MAKE_KEY_INFO(SPStreamAbility_t, KEY_TYPE_OBJECT, resList	 	, json_stream_resolution),
	MAKE_KEY_INFO(SPStreamAbility_t, KEY_TYPE_U32, resListCnt	 	, NULL),
	//}end added
	MAKE_KEY_INFO(SPStreamAbility_t, KEY_TYPE_U32, maxNGOP	 	, NULL),
	MAKE_KEY_INFO(SPStreamAbility_t, KEY_TYPE_U32, minNGOP	 	, NULL),
	MAKE_KEY_INFO(SPStreamAbility_t, KEY_TYPE_U32, maxFramerate		, NULL),
	MAKE_KEY_INFO(SPStreamAbility_t, KEY_TYPE_U32, minFramerate	 	, NULL),
	MAKE_KEY_INFO(SPStreamAbility_t, KEY_TYPE_U32, maxKBitrate	 	, NULL),
	MAKE_KEY_INFO(SPStreamAbility_t, KEY_TYPE_U32, minKBitrate	 	, NULL),
	MAKE_KEY_INFO(SPStreamAbility_t, KEY_TYPE_OBJECT, inputRes	 	, json_stream_resolution),
	//begin added by zhouwq 20150706 {
	//MAKE_ARRAY_INFO(jvstream_ability_t, KEY_TYPE_ARRAY, maxStreamRes, NULL, MAX_STREAM, KEY_TYPE_U32),
	MAKE_KEY_INFO(SPStreamAbility_t, KEY_TYPE_U32, maxRoi	 	, NULL),
	//}end added
	
	MAKE_END_INFO()
};

#define webstream_GENERAL	"stream operation command"
#define webstream_DETAIL	\
	"\nUsage: webstream STREAMID OPERATION [PARAM]\n"\
	"      webstream -c[CHANNELID] STREAMID OPERATION [PARAM]"\
	"CHANNELID: \n"\
	"  The ID such as 1,2,3,4... For IPC, It is 1 always\n"\
	"STREAMID: \n"\
	"  The ID such as 1,2,3 Set the stream setting of CHANNELID. \n"\
	"\n"\
	"Operations:\n"\
	"  list\r\t\tList the stream info\n"\
	"  set\r\t\tSet the stream info with PARAM\n"\
	"  resolution\r\tList the stream resolution list\n"\
	"  ability\r\tList the enc ability\n"\
	"\n"\
	"PARAM:\n"\
	"  PARAM is setted when OPTION is \"set\"\n"\
	"  You can use cmd: 'webstream list' to see the format of PARAM\n"\
	"Example:\n"\
	"  webstream 1 set '{\"bEnable\":1,\"width\":1920,\"height\":1080,\"framerate\":25,\"nGOP\":50,\"bitrate\":2048}'\n"\
	"  webstream -c1 1 set '{\"bEnable\":1,\"width\":720,\"height\":480,\"framerate\":25,\"nGOP\":50,\"bitrate\":512}'\n"\
	"\n"

int webcmd_stream(int argc, char *argv[])
{
	//mstream_attr_t stream, *mp;
	SPStreamAttr_t stream, *mp;
	char *out;
	int channelid;
	int streamid;
	int ret;

	//加入用户鉴权后，参数个数多两个
	//if (argc < 3)
	if (argc < ((CheckPower == 1)?5:3))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("param error");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[2],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}

	channelid = 1;
	ret = sscanf(argv[1], "-c%d", &channelid);
	if (ret == 1)// -c[CHANNELID]
		argv++;

	streamid = atoi(argv[1])-1;
	//mstream_get_param(streamid, &stream);
	memset(&stream,0,sizeof(SPStreamAttr_t));
	sp_stream_get_param(streamid,&stream);
	if (strcmp(argv[2], "list") == 0)
	{
		out = cjson_object2string(json_stream,&stream);
		if (out != NULL)
		{
			weber_set_result(out);
			free(out);
			return 0;
		}
		else
		{
			weber_set_result("unknown error");
			return -1;
		}
	}
	else if (strcmp(argv[2], "set") == 0)
	{
		//加入用户鉴权后，参数个数多两个
		//if (argc != 4)
		if (argc != ((CheckPower == 1)?6:4))
		{
			printf("the number of params isn't right , argc=%d\n",argc);
			weber_set_result("param error");
			return -1;
		}
		mp = cjson_string2object(json_stream, argv[3], &stream);
		if (mp == NULL)
		{
			weber_set_result("param error");
			return -1;
		}
		//mstream_set_param(streamid, &stream);
		//mstream_flush(streamid);
		sp_stream_set_param(streamid,&stream);
		weber_set_result("ok");
		//_webcmd_save();
	}
	else if (strcmp(argv[2], "resolution") == 0)
	{
		//jvstream_ability_t ability;
		SPStreamAbility_t ability;

		//jv_stream_get_ability(streamid,&ability);
		memset(&ability,0,sizeof(SPStreamAbility_t));
		sp_stream_get_ability(streamid,&ability);
		out = cjson_object_array2string(json_stream_resolution,ability.resList, ability.resListCnt);
		if (out != NULL)
		{
			weber_set_result(out);
			free(out);
			return 0;
		}
		else
		{
			weber_set_result("unknown error");
			return -1;
		}
	}
	else if (strcmp(argv[2], "ability") == 0)
	{
		//jvstream_ability_t ability;
		SPStreamAbility_t ability;

		//jv_stream_get_ability(streamid,&ability);
		memset(&ability,0,sizeof(SPStreamAbility_t));
		sp_stream_get_ability(streamid,&ability);
		out = cjson_object2string(json_stream_ability,&ability);
		if (out != NULL)
		{
			weber_set_result(out);
			free(out);
			return 0;
		}
		else
		{
			weber_set_result("unknown error");
			return -1;
		}
	}
	else if (strcmp(argv[2], "requestidr") == 0)
	{
		sp_stream_request_idr(streamid);
		weber_set_result("request finished");
	}
	return 0;
}

#define webifconfig_GENERAL	"ifconfig operation command"
#define webifconfig_DETAIL	\
	"\nUsage: webifconfig <list|set>  [PARAM]\n"\
	"Operations:\n"\
	"  list\r\t\tList the network info\n"\
	"  set\r\t\tSet the network info with PARAM\n"\
	"\n"\
	"PARAM:\n"\
	"  PARAM is setted when OPTION is \"set\"\n"\
	"  You can use cmd: 'webifconfig list' to see the format of PARAM\n"\
	"Example:\n"\
	"  webifconfig set {\"inet\":\"dhcp\",\"eth\":{\"name\":\"eth0\",\"bDHCP\":1,\"addr\":\"\",\"mask\":\"\",\"gateway\":\"0.0.0.0\",\"mac\":\"02:00:01:01:01:12\",\"dns\":\"8.8.8.8\"},\"pppoe\":{\"name\":\"ppp0\",\"username\":\"x\",\"passwd\":\"1\"}}\n"\
	"\n"

static json_kinfo_t json_eth[] =
{
	MAKE_KEY_INFO(eth_t, KEY_TYPE_STRING, name 	, NULL),
	MAKE_KEY_INFO(eth_t, KEY_TYPE_U32, bDHCP 	, NULL),
	MAKE_KEY_INFO(eth_t, KEY_TYPE_STRING, addr 	, NULL),
	MAKE_KEY_INFO(eth_t, KEY_TYPE_STRING, mask 	, NULL),
	MAKE_KEY_INFO(eth_t, KEY_TYPE_STRING, gateway 	, NULL),
	MAKE_KEY_INFO(eth_t, KEY_TYPE_STRING, mac 	, NULL),
	MAKE_KEY_INFO(eth_t, KEY_TYPE_STRING, dns 	, NULL),

	MAKE_END_INFO()
};

static json_kinfo_t json_pppoe[] =
{
	MAKE_KEY_INFO(pppoe_t, KEY_TYPE_STRING, name 	, NULL),
	MAKE_KEY_INFO(pppoe_t, KEY_TYPE_STRING, username 	, NULL),
	MAKE_KEY_INFO(pppoe_t, KEY_TYPE_STRING, passwd 	, NULL),

	MAKE_END_INFO()
};

static json_kinfo_t json_wifi[] =
{
	MAKE_KEY_INFO(wifiap_t, KEY_TYPE_STRING, name, NULL),
	MAKE_KEY_INFO(wifiap_t, KEY_TYPE_STRING, passwd, NULL),
	MAKE_KEY_INFO(wifiap_t, KEY_TYPE_U32, quality		, NULL),
	MAKE_KEY_INFO(wifiap_t, KEY_TYPE_U32, keystat		, NULL),
	MAKE_KEY_INFO(wifiap_t, KEY_TYPE_STRING, iestat, NULL),

	MAKE_END_INFO()
};

typedef struct{
	char inet[8];//static/dhcp/ppp/wifi
	eth_t eth;
	pppoe_t pppoe;
	wifiap_t wifiap;
}ifconfig_info_t;

static json_kinfo_t json_ifconfig[] =
{
	MAKE_KEY_INFO(ifconfig_info_t, KEY_TYPE_STRING, inet 	, NULL),
	MAKE_KEY_INFO(ifconfig_info_t, KEY_TYPE_OBJECT, eth	, json_eth),
	MAKE_KEY_INFO(ifconfig_info_t, KEY_TYPE_OBJECT, pppoe, json_pppoe),
	MAKE_KEY_INFO(ifconfig_info_t, KEY_TYPE_OBJECT, wifiap, json_wifi),

	MAKE_END_INFO()
};

int webcmd_ifconfig(int argc, char *argv[])
{//window.location.href="http://www.baidu.com";return false;
	int cnt;
	int ret;
	char *out;
	ifconfig_info_t ifconfig;
	char current[12];
	char iface[16] = {0x00};
	wifiap_t *wifiTmp;

	//加入用户鉴权，参数个数多两个
	//if (argc == 1)
	if (argc < ((CheckPower == 1)?4:2))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("param error");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[1],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}
	
	if (strcmp("list", argv[1]) == 0)
	{
		memset(&ifconfig, 0, sizeof(ifconfig_info_t));
		//utl_ifconfig_get_inet(ifconfig.inet);
		sp_ifconfig_get_inet(ifconfig.inet);
		if (strcmp(ifconfig.inet, "ppp") == 0)
		{
			mlog_write("Network actived PPPOE");
			strcpy(iface, "ppp0");
			ret = utl_ifconfig_ppp_get(&ifconfig.pppoe);
		}
		else if (strcmp(ifconfig.inet, "wifi") == 0)
		{
			mlog_write("Network actived WIFI");
			//utl_ifconfig_get_iface(iface);
			sp_ifconfig_get_iface(iface);
			ret = utl_ifconfig_wifi_get_current(&ifconfig.wifiap);
		}
		else //if (strcmp(ifconfig.inet, "static") == 0)
		{
			mlog_write("Network actived Ethernet");
			if (strcmp(ifconfig.inet, "static") == 0)
				mlog_write("DHCP Enabled");
			else
				mlog_write("DHCP Disabled");
			strcpy(iface, "eth0");
		}
		utl_ifconfig_build_attr(iface,&ifconfig.eth, FALSE);

		out = cjson_object2string(json_ifconfig,&ifconfig);
		jv_assert(out, return -1;);
		weber_set_result(out);
		free(out);
		return 0;
	}
	else if (strcmp("set", argv[1]) == 0)
	{
		memset(&ifconfig, 0, sizeof(ifconfig));
		cjson_string2object(json_ifconfig,argv[2],&ifconfig);
		if (strcmp(ifconfig.inet, "ppp") == 0)
		{
			ret = utl_ifconfig_ppp_set(&ifconfig.pppoe);
		}
		else if (strcmp(ifconfig.inet, "wifi") == 0)
		{
			if(ifconfig.wifiap.name[0] != '\0' && ifconfig.wifiap.iestat[0] == '\0')
			{
				wifiTmp = utl_ifconfig_wifi_get_by_ssid(ifconfig.wifiap.name);
				ifconfig.wifiap.iestat[0] = wifiTmp->iestat[0];
				ifconfig.wifiap.iestat[1] = wifiTmp->iestat[1];
			}
			ret = utl_ifconfig_wifi_connect(&ifconfig.wifiap);
		}
		else
			ret = utl_ifconfig_eth_set(&ifconfig.eth);
		if (ret == 0)
			weber_set_result("ok");
		return 0;
	}
	else if (strcmp("scan", argv[1]) == 0)
	{
		wifiap_t *list;
		if (utl_ifconfig_wifi_get_mode() == WIFI_MODE_STA)
		{
			list = utl_ifconfig_wifi_list_ap(TRUE);
		}
		else
		{
			list = utl_ifconfig_wifi_power_list_ap();
		}
		cnt = utl_ifconfig_wifi_list_cnt(list);

		out = cjson_object_array2string(json_wifi,list,cnt);
		jv_assert(out, {return -1;});
		weber_set_result(out);
		free(out);
		return 0;
	}


/*
	if (argc == 1)
	{
		weber_set_result(utl_ifconfig_get_current());
	}
	else if (strcmp("eth", argv[1]) == 0)
	{
		if (strcmp("list", argv[2]) == 0)
		{
			memset(&attr, 0, sizeof(eth_t));
			ret = utl_ifconfig_eth_get(&attr);
			if (ret != 0)
				return -1;
			out = cjson_object2string(json_eth,&attr);
			jv_assert(out, return -1;);
			weber_set_result(out);
			free(out);
			return 0;
		}
		else if (strcmp("set", argv[2]) == 0)
		{
			cjson_string2object(json_eth,argv[3],&attr);
			ret = utl_ifconfig_eth_set(&attr);
			if (ret == 0)
				weber_set_result("ok");
		}
		return 0;
	}
	else if (strcmp("pppoe", argv[1]) == 0)
	{
		pppoe_t ppp;
		if (strcmp("list", argv[2]) == 0)
		{
			memset(&ppp, 0, sizeof(pppoe_t));
			ret = utl_ifconfig_ppp_get(&ppp);
			if (ret != 0)
				return -1;
			out = cjson_object2string(json_pppoe,&ppp);
			jv_assert(out, return -1;);
			weber_set_result(out);
			free(out);
			return 0;
		}
		else if (strcmp("set", argv[2]) == 0)
		{
			cjson_string2object(json_pppoe,argv[3],&ppp);
			ret = utl_ifconfig_ppp_set(&ppp);
			if (ret == 0)
				weber_set_result("ok");
		}
		return 0;
	}
	*/
	return 0;
}

#define webwifi_GENERAL	"wifi operation command"
#define webwifi_DETAIL	\
	"\nUsage: wifi OPERATION [PARAM]\n"\
	"Operations:\n"\
	"  list\r\t\tList the wifi ap list\n"\
	"  connect\r\t\tconnect the wifi ap with PARAM\n"\
	"\n"\
	"PARAM:\n"\
	"  PARAM is setted when OPTION is \"connect\"\n"\
	"  You can use cmd: 'webwifi list' to see the format of PARAM\n"\
	"Example:\n"\
	"  \n"\
	"\n"
	
int webcmd_wifi(int argc, char *argv[])
{
	int cnt;
	int ret;
	char *out;
	wifiap_t wifiap,*wifiTmp;
	wifiap_t *list;
	char current[12];
	int index = 0;

	//加入用户鉴权后，参数个数多两个
	//if (argc == 1)
	if (argc < ((CheckPower == 1)?4:2))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("param error");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[1],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}
	
	if (strcmp("list", argv[1]) == 0)
	{
		if (utl_ifconfig_wifi_bsupport())
		{
			list = utl_ifconfig_wifi_get_ap();
			cnt = utl_ifconfig_wifi_list_cnt(list);
		}
		else
		{
			list= NULL;
			cnt=0;
		}
		out = cjson_object_array2string(json_wifi,list,cnt);
		jv_assert(out, {return -1;});
		weber_set_result(out);
		free(out);
		return 0;
	}
	else if (strcmp("connect", argv[1]) == 0)
	{
		if (utl_ifconfig_wifi_bsupport())
		{
			memset(&wifiap, 0, sizeof(wifiap));
			cjson_string2object(json_wifi,argv[2],&wifiap);
			
			if(wifiap.name[0] != '\0' && wifiap.iestat[0] == '\0')
			{
				wifiTmp = utl_ifconfig_wifi_get_by_ssid(wifiap.name);
				wifiap.iestat[0] = wifiTmp->iestat[0];
				wifiap.iestat[1] = wifiTmp->iestat[1];
			}
			//utl_ifconfig_wifi_start_sta();
			sp_ifconfig_wifi_start_sta();
			//net_deinit();
			sp_ifconfig_net_deinit();
			ret = utl_ifconfig_wifi_connect(&wifiap);
			if (ret == 0)
			{
				weber_set_result("ok");
			}
		}
	}
	else if(strcmp("changemode", argv[1]) == 0)
	{
		char strRlt[40] = {0};
		if(utl_ifconfig_wifi_get_mode() == WIFI_MODE_STA)
		{
			sp_ifconfig_wifi_start_ap();
			sprintf(strRlt,"change wifi mode from STA to AP ");
		}
		else if(utl_ifconfig_wifi_get_mode() == WIFI_MODE_AP)
		{
			sp_ifconfig_wifi_start_sta();
			sp_ifconfig_net_deinit();
			utl_ifconfig_wifi_connect(NULL);
			sprintf(strRlt,"change wifi mode from AP to STA ");
		}
		weber_set_result(strRlt);
	}

	return 0;
}

static json_kinfo_t json_osd[] =
{
	MAKE_KEY_INFO(SPChnOsdAttr_t, KEY_TYPE_U32, bShowOSD			, NULL),
	MAKE_KEY_INFO(SPChnOsdAttr_t, KEY_TYPE_STRING, timeFormat		, NULL),
	MAKE_KEY_INFO(SPChnOsdAttr_t, KEY_TYPE_U32, position	 			, NULL),
	//begin added by zhouwq 20150706 {
	MAKE_KEY_INFO(SPChnOsdAttr_t, KEY_TYPE_U32, timePos	 			, NULL),
	//}end added
	MAKE_KEY_INFO(SPChnOsdAttr_t, KEY_TYPE_STRING, channelName	 	, NULL),
	//begin added by zhouwq 20150706 {
	MAKE_KEY_INFO(SPChnOsdAttr_t, KEY_TYPE_U32, osdbInvColEn	 		, NULL),
	MAKE_KEY_INFO(SPChnOsdAttr_t, KEY_TYPE_U32, bLargeOSD	 			, NULL),
	//}end added

	MAKE_END_INFO()
};

#define webosd_GENERAL	"osd operation command"
#define webosd_DETAIL	\
	"\nUsage: webosd CHANNELID OPERATION [PARAM]\n"\
	"CHANNELID: \n"\
	"  The ID such as 1,2,3,4... For IPC, It is 1 always\n"\
	"\n"\
	"Operations:\n"\
	"  list\r\t\tList the osd info\n"\
	"  set\r\t\tSet the osd info with PARAM\n"\
	"\n"\
	"PARAM:\n"\
	"  PARAM is setted when OPTION is \"set\"\n"\
	"  You can use cmd: 'webosd list' to see the format of PARAM\n"\
	"Example:\n"\
	"  webosd 1 set '{\"bEnable\":1,\"width\":1920,\"height\":1080,\"framerate\":25,\"nGOP\":50,\"bitrate\":2048}'\n"\
	"\n"

int webcmd_osd(int argc, char *argv[])
{
	//mchnosd_attr osd, *mp;
	SPChnOsdAttr_t osd, *mp;
	char *out;
	int channelid;

	//加入用户鉴权，参数个数多两个
	//if (argc < 3)
	if (argc < ((CheckPower == 1)?5:3))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("param error");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[2],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}

	channelid = atoi(argv[1])-1;
	//mchnosd_get_param(channelid, &osd);
	memset(&osd,0,sizeof(SPChnOsdAttr_t));
	sp_chnosd_get_param(channelid,&osd);
	if (strcmp(argv[2], "list") == 0)
	{
		out = cjson_object2string(json_osd,&osd);
		if (out != NULL)
		{
			weber_set_result(out);
			free(out);
			return 0;
		}
		else
		{
			weber_set_result("unknown error");
			return -1;
		}
	}
	else if (strcmp(argv[2], "set") == 0)
	{
		//加入用户鉴权，参数个数多两个
		//if (argc != 4)
		if (argc != ((CheckPower == 1)?6:4))
		{
			printf("the number of params isn't right , argc=%d\n",argc);
			weber_set_result("param error");
			return -1;
		}
		mp = cjson_string2object(json_osd, argv[3], &osd);
		if (mp == NULL)
		{
			weber_set_result("param error");
			return -1;
		}
		//mchnosd_set_param(channelid, &osd);
		//mchnosd_flush(channelid);
		sp_chnosd_set_param(channelid,&osd);
		weber_set_result("ok");
		if(strcmp(osd.timeFormat,"YYYY-MM-DD hh:mm:ss") == 0)
		{
			gp.nTimeFormat = 1;
		}
		else
		{
			gp.nTimeFormat = 0;
		}
		_webcmd_save();
	}
	return 0;
}


typedef enum{
	JSSence_Defalt,
	JSSence_InDoor,
	JSSence_OutDOor,
	JSSence_Soft,
}JSSence_e;

typedef struct{
	int brightness;
	int contrast;
	int saturation;
	int sharpness;

	JSSence_e sence;

	char bAutoAWB; //auto white balance
	char bMirror;
	char bFlip;
	char bNoColor;
	char bWDR; //开启宽动态
}JSImage_t;

static json_kinfo_t json_image[] =
{
	MAKE_KEY_INFO(JSImage_t, KEY_TYPE_U32, brightness			, NULL),
	MAKE_KEY_INFO(JSImage_t, KEY_TYPE_U32, contrast			, NULL),
	MAKE_KEY_INFO(JSImage_t, KEY_TYPE_U32, saturation			, NULL),
	MAKE_KEY_INFO(JSImage_t, KEY_TYPE_U32, sharpness			, NULL),
	MAKE_KEY_INFO(JSImage_t, KEY_TYPE_U32, sence			, NULL),
	MAKE_KEY_INFO(JSImage_t, KEY_TYPE_U8, bAutoAWB, NULL),
	MAKE_KEY_INFO(JSImage_t, KEY_TYPE_U8, bMirror, NULL),
	MAKE_KEY_INFO(JSImage_t, KEY_TYPE_U8, bFlip, NULL),
	MAKE_KEY_INFO(JSImage_t, KEY_TYPE_U8, bNoColor, NULL),
	MAKE_KEY_INFO(JSImage_t, KEY_TYPE_U8, bWDR, NULL),

	MAKE_END_INFO()
};

#define webimage_GENERAL	"osd operation command"
#define webimage_DETAIL	\
	"\nUsage: webosd CHANNELID OPERATION [PARAM]\n"\
	"CHANNELID: \n"\
	"  The ID such as 1,2,3,4... For IPC, It is 1 always\n"\
	"\n"\
	"Operations:\n"\
	"  list\r\t\tList the osd info\n"\
	"  set\r\t\tSet the osd info with PARAM\n"\
	"\n"\
	"PARAM:\n"\
	"  PARAM is setted when OPTION is \"set\"\n"\
	"  You can use cmd: 'webosd list' to see the format of PARAM\n"\
	"Example:\n"\
	"  webosd 1 set '{\"bEnable\":1,\"width\":1920,\"height\":1080,\"framerate\":25,\"nGOP\":50,\"bitrate\":2048}'\n"\
	"\n"

int webcmd_image(int argc, char *argv[])
{
	mchnosd_attr osd, *mp;
	char *out;
	int channelid;

	if (argc < 3)
	{
		weber_set_result("param error");
		return -1;
	}

	channelid = atoi(argv[1])-1;
	mchnosd_get_param(channelid, &osd);
	if (strcmp(argv[2], "list") == 0)
	{
		out = cjson_object2string(json_osd,&osd);
		if (out != NULL)
		{
			weber_set_result(out);
			free(out);
			return 0;
		}
		else
		{
			weber_set_result("unknown error");
			return -1;
		}
	}
	else if (strcmp(argv[2], "set") == 0)
	{
		if (argc != 4)
		{
			weber_set_result("param error");
			return -1;
		}
		mp = cjson_string2object(json_osd, argv[3], &osd);
		if (mp == NULL)
		{
			weber_set_result("param error");
			return -1;
		}
		mchnosd_set_param(channelid, &osd);
		mchnosd_flush(channelid);
		weber_set_result("ok");
		if(strcmp(osd.timeFormat,"YYYY-MM-DD hh:mm:ss") == 0)
		{
			gp.nTimeFormat = 1;
		}
		else
		{
			gp.nTimeFormat = 0;
		}
		_webcmd_save();
	}
	return 0;
}

static json_kinfo_t json_yst[] =
{
	MAKE_KEY_INFO(YST, KEY_TYPE_STRING, strGroup	, NULL),
	MAKE_KEY_INFO(YST, KEY_TYPE_U32, nID			, NULL),
	MAKE_KEY_INFO(YST, KEY_TYPE_U32, nPort			, NULL),
	MAKE_KEY_INFO(YST, KEY_TYPE_U32, nStatus	 	, NULL),
	MAKE_KEY_INFO(YST, KEY_TYPE_U32, nYSTPeriod		, NULL),
	MAKE_KEY_INFO(YST, KEY_TYPE_STRING, bTransmit	, NULL),
	MAKE_KEY_INFO(YST, KEY_TYPE_U32, eLANModel		, NULL),

	MAKE_END_INFO()
};

#define webyst_GENERAL	"yst operation command"
#define webyst_DETAIL	\
	"\nUsage: webyst OPERATION [PARAM]\n"\
	"\n"\
	"Operations:\n"\
	"  list\r\t\tList the yst info\n"\
	"\n"\
	"PARAM:\n"\
	"  PARAM is setted when OPTION is \"set\"\n"\
	"  You can use cmd: 'webosd list' to see the format of PARAM\n"\
	"Example:\n"\
	"  webyst list\n"\
	"\n"


int webcmd_yst(int argc, char *argv[])
{
	YST yst, *mp;
	char *out;
	
	//if (argc < 2)
	if (argc < ((CheckPower == 1)?4:2))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("{\"status\":\"unknown error\",\"data\":\"\"}");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[1],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}

#ifdef YST_SVR_SUPPORT	
	GetYSTParam(&yst);
	if (strcmp(argv[1], "list") == 0)
	{
		out = cjson_object2string(json_yst,&yst);
		if (out != NULL)
		{
			weber_set_result(out);
			free(out);
			return 0;
		}
		else
		{
			weber_set_result("unknown error");
			return -1;
		}
	}
	if (strcmp(argv[1], "get_port") == 0)
	{
		char port[64];
		//前面鉴权过了，这里删掉
		//int ret = maccount_check_power_ex(argv[argc-2],argv[argc-1]);
		//if(ret==ERR_NOTEXIST_EX || ret==ERR_PASSWD_EX)
		//{
		//	weber_set_result("{\"status\":\"username or password error\",\"data\":\"\"}");
		//	return -1;
		//}
		if (GetYSTParam(&yst) != NULL)
		{
			sprintf(port,"{\"status\":\"ok\",\"data\":{\"nPort\": %d}}",yst.nPort);
			//{\"nPort\": %d}
			weber_set_result(port);
			return 0;
		}
		else
		{
			weber_set_result("{\"status\":\"unknown error\",\"data\":\"\"}");
			return -1;
		}
	}
	if (strcmp(argv[1], "get_video") == 0)
	{
		char video[1024], iface[32];
		//int ret;
		eth_t eth;
		//前面鉴权过了，这里删掉
		//ret = maccount_check_power_ex(argv[argc-2],argv[argc-1]);
		//if(ret==ERR_NOTEXIST_EX || ret==ERR_PASSWD_EX)
		//{
		//	weber_set_result("{\"status\":\"username or password error\",\"data\":\"\"}");
		//	return -1;
		//}

		utl_ifconfig_get_iface(iface);
		utl_ifconfig_build_attr(iface,&eth, FALSE);
		if (GetYSTParam(&yst) != NULL)
		{
			sprintf(video,
					"{\"status\":\"ok\",\"data\":[{\"id\": %d,\"stream%d\":\"rtsp://%s/live%d.264\",\"stream%d\":\"rtsp://%s/live%d.264\"}]}",
							1,0,eth.addr,0,
							  1,eth.addr,1);
			weber_set_result(video);
			return 0;
		}
		else
		{
			weber_set_result("{\"status\":\"unknown error\",\"data\":\"\"}");
			return -1;
		}
	}
#else
	weber_set_result("unknown error");
#endif
    return 0;
}



#define websnapshot_GENERAL	"Have a snapshot of the channel"
#define websnapshot_DETAIL	\
	"\nUsage: snapshot CHANNELID FNAME\n"\
	"CHANNELID: \n"\
	"  The ID such as 1,2,3,4... For IPC, It is 1 always\n"\
	"FNAME:\n"\
	"  The file name which will store the picture\n"\
	"\n"
int webcmd_snapshot(int argc, char *argv[])
{
	FILE *fp;
	int len;
	unsigned char *data;
	char *fname;
	int paramNum = 3;

	if (argc != paramNum)
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("param error");
		return -1;
	}

	if(strcmp(argv[2], "snapshot") != 0)
	{
		//用户鉴权
		if(0 != _webcmd_checkpower(argv[0],argv[2],argv[argc-2],argv[argc-1]))
		{
			printf("user power check failed \n");
			return -1;
		}
	}
	/*
	fname = argv[2];
	data = malloc(256*1024);
	len = msnapshot_get(atoi(argv[1])-1, data,256*1024);
	fp = fopen(fname,"wb");
	if (fp == NULL)
	{
		free(data);
		weber_set_result("failed snapshot");
		return -1;
	}
	printf("len: %d\n", len);
	fwrite(data, 1, len, fp);
	free(data);
	fclose(fp);
	weber_set_result("ok");
	*/
	if (strcmp(argv[2], "stream") == 0)
	{
		char uri[128];
		SPEth_t eth;

		sp_ifconfig_eth_get(&eth);
		sprintf(uri, "http://%s/%s", eth.addr, "cgi-bin/getsnapshot.cgi");
		weber_set_result(uri);
	}
	else if (strcmp(argv[2], "snapshot") == 0)
	{
		int ret = msnapshot_get_shmdata(0);
		if(ret == 0)
		{
			weber_set_result("snapshot ok");
		}
		else
		{
			weber_set_result("snapshot fail");
		}
	}
	else if(strcmp(argv[2], "save") == 0)
	{
		time_t nsecond;
		struct tm *tm_snap;
		char tmpPath[100] = {0};
		
		if (0 != access("/progs/rec/00/", F_OK))  //查看TF卡是否挂载成功
		{
			printf("cannot access sd card\n");
			weber_set_result("abnormal status of sd card ");
			return -1;
		}

		if(0 != access("/progs/rec/00/snapshot/capture/", F_OK))
		{
			printf("cannot access the dest path :/progs/rec/00/snapshot/capture/\n");
			weber_set_result("cannot access the dest sd path :snapshot/capture/");
			return -1;
		}
		
		nsecond = time(NULL);
		tm_snap = localtime(&nsecond);
		sprintf(tmpPath,"/progs/rec/00/snapshot/capture/%04d%02d%02d_%02d%02d%02d.jpg",
			tm_snap->tm_year+1900,
			tm_snap->tm_mon+1,
			tm_snap->tm_mday,
			tm_snap->tm_hour,
			tm_snap->tm_min,
			tm_snap->tm_sec);
		sp_snapshot(atoi(argv[1])-1, tmpPath);
		weber_set_result(tmpPath);
	}
	else if(strcmp(argv[2], "enable") == 0)
	{
		if (0 != access("/progs/rec/00/", F_OK))
		{
			printf("cannot access sd card\n");
			weber_set_result("abnormal status of sd card ");
			return -1;
		}
		mkdir("/progs/rec/00/snapshot/", 0777);
		mkdir("/progs/rec/00/snapshot/capture/", 0777);
		mkdir("/progs/rec/00/snapshot/timelapse/", 0777);
		if(0 != access("/progs/rec/00/snapshot/capture/", F_OK)
			|| 0 != access("/progs/rec/00/snapshot/timelapse/", F_OK))
		{
			weber_set_result("error happens");
			return -1;
		}
		weber_set_result("ok");
	}
	return 0;
}

#define webhelp_GENERAL	"Display help info"
#define webhelp_DETAIL	\
	"\nUsage: webhelp [cmd]\n"\
	"  webhelp get the cmd list, and webhelp cmd get the help detail of cmd \n"\
	"\n"

int webcmd_help(int argc, char *argv[])
{
	char *cmd = NULL;
	char *help;

	if(argc < ((CheckPower == 1)?3:1))
	{
		printf("the number of param isn't right , argc=%d\n",argc);
		weber_set_result("param error");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],NULL,argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}

	//加入用户鉴权后，参数多两个
	//if (argc == 2)
	if (argc == ((CheckPower == 1)?4:2))
		cmd = argv[1];
	help = utl_cmd_get_help(cmd);
	weber_set_result(help);
	free(help);
	return 0;
}

static json_kinfo_t json_ipcinfo[] =
{
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_STRING, type, NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_STRING, product, NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_STRING, version, NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_STRING, acDevName, NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_STRING, nickName, NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_U32, sn, NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_U32, ystID, NULL),
	MAKE_ARRAY_INFO(ipcinfo_t, KEY_TYPE_ARRAY, nDeviceInfo	, NULL,9,KEY_TYPE_U32),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_U32, nLanguage, NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_STRING, date, NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_U32, bSntp, NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_U32, sntpInterval, NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_STRING, ntpServer, NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_U32, tz, NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_U32, bDST, NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_U32, rebootDay, 		NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_U32, rebootHour, 		NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_U32, bRestriction, 	NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_STRING, portUsed, 	NULL),
	MAKE_ARRAY_INFO(ipcinfo_t, KEY_TYPE_ARRAY, osdText, NULL, 6, KEY_TYPE_STRING),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_U32, osdX, 			NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_U32, osdY, 			NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_U32, osdSize, 		NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_STRING, lcmsServer, 	NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_U32, bWebServer,		NULL),
	MAKE_KEY_INFO(ipcinfo_t, KEY_TYPE_U32, nWebPort,		NULL),
	//}end added

	MAKE_END_INFO()
};

#define webipcinfo_GENERAL	"ipcinfo operation command"
#define webipcinfo_DETAIL	\
	"\nUsage: webipcinfo OPERATION [PARAM]\n"\
	"Operations:\n"\
	"  list\r\t\tList the ipc info\n"\
	"  set\r\t\tSet the ipc info with PARAM\n"\
	"  settime\r\t\tSet the ipc time. webipcinfo settime 2012-06-07 13:58:00\n"\
	"  system\r\t\tSystem control, PARAM just:[reboot/reset/softreset]\n"\
	"\n"\
	"PARAM:\n"\
	"  PARAM is setted when OPTION is \"set\"\n"\
	"  You can use cmd: 'webipcinfo list' to see the format of PARAM\n"\
	"Example:\n"\
	"  webipcinfo set '{\"acDevName\":\"ipcam-1\"}'\n"\
	"\n"

int webcmd_ipcinfo(int argc, char *argv[])
{
	ipcinfo_t ipcinfo, *mp;
	char *out;
	printf("running...%d\n", __LINE__);
	//加入用户鉴权后，参数个数多两个
	//if (argc < 2)
	if (argc < ((CheckPower == 1)?4:2))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("param error");
		return -1;
	}
	printf("running...%d\n", __LINE__);

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[1],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}

	ipcinfo_get_param(&ipcinfo);
	if (strcmp(argv[1], "list") == 0)
	{
		out = cjson_object2string(json_ipcinfo,&ipcinfo);
		if (out != NULL)
		{
			weber_set_result(out);
			free(out);
			return 0;
		}
		else
		{
			weber_set_result("unknown error");
			return -1;
		}
	}
	else if (strcmp(argv[1], "set") == 0)
	{
		//加入用户鉴权后，参数个数多两个
		//if (argc != 3)
		if (argc != ((CheckPower == 1)?5:3))
		{
			Printf("test argc: %d, param:%s\n", argc,argv[3]);
			weber_set_result("param error");
			return -1;
		}
		Printf("test\n");
		mp = cjson_string2object(json_ipcinfo, argv[2], &ipcinfo);
		if (mp == NULL)
		{
			Printf("test\n");
			weber_set_result("param error");
			return -1;
		}
		ipcinfo_set_param(&ipcinfo);
		weber_set_result("ok");
		_webcmd_save();
	}
	else if (strcmp(argv[1], "settime") == 0)
	{
		//加入用户鉴权后，参数个数多两个
		//if (argc != 3)
		if (argc != ((CheckPower == 1)?5:3))
		{
			Printf("test argc: %d, param:%s\n", argc,argv[3]);
			weber_set_result("param error");
			return -1;
		}

		ipcinfo_set_time(argv[2]);

		weber_set_result("ok");
		_webcmd_save();
	}
	else if (strcmp(argv[1], "system") == 0)
	{
		printf("running...%d\n", __LINE__);
		//加入用户鉴权后，参数个数多两个
		//if (argc != 3)
		if (argc != ((CheckPower == 1)?5:3))
		{
			Printf("test argc: %d, param:%s\n", argc,argv[3]);
			weber_set_result("param error");
			return -1;
		}

		if (strcmp(argv[2], "reboot") == 0)
		{
			SYSFuncs_reboot();
		}
		else if (strcmp(argv[2], "reset") == 0)
		{
			SYSFuncs_factory_default(1);
		}
		else if (strcmp(argv[2], "softreset") == 0)
		{
			SYSFuncs_factory_default(0);
		}
		else
		{
			weber_set_result("param error");
			return -1;
		}
		printf("running...%d\n", __LINE__);

		weber_set_result("ok");
	}
	else
	{
		Printf("cmd not reco....\n");
	}
	return 0;
}

#if 1
typedef struct{
	//fixed info
	char type[8];	//ipc/nvr/dvr
	char hardware[32];	//硬件名称
	char firmware[32];	//软件版本
	char manufacture[32]; //生产厂商
	char sn[32];		//SN
	char model[32];

	int channelCnt;
	int streamCnt;

	//N通道M码流的云视通通道号为：ystChannelNo[N*channelCnt+M+1];
	//注意，其值从1开始。0表示无服务
	unsigned short ystChannelNo[128];

	//can be setted
	char name[32];		//机器名称
	char date[32];
	BOOL bSntp;	//自动网络校时
	char ntpServer[64];//自动对时的网络服务器
	int sntpInterval;//自动网络校时间隔时间，单位为小时
	int tz;	//时区  -12 ~ 13
	int bDST; //Daylight Saving Time 夏令时
}js_devinfo_t;

static json_kinfo_t json_devinfo[] =
{
	MAKE_KEY_INFO(js_devinfo_t, KEY_TYPE_STRING, type, NULL),
	MAKE_KEY_INFO(js_devinfo_t, KEY_TYPE_STRING, hardware, NULL),
	MAKE_KEY_INFO(js_devinfo_t, KEY_TYPE_STRING, firmware, NULL),
	MAKE_KEY_INFO(js_devinfo_t, KEY_TYPE_STRING, manufacture, NULL),
	MAKE_KEY_INFO(js_devinfo_t, KEY_TYPE_STRING, sn, NULL),
	MAKE_KEY_INFO(js_devinfo_t, KEY_TYPE_STRING, model, NULL),

	MAKE_KEY_INFO(js_devinfo_t, KEY_TYPE_U32, channelCnt, NULL),
	MAKE_KEY_INFO(js_devinfo_t, KEY_TYPE_U32, streamCnt, NULL),
	MAKE_ARRAY_INFO(js_devinfo_t, KEY_TYPE_ARRAY, ystChannelNo	, NULL,128,KEY_TYPE_U16),

	MAKE_KEY_INFO(js_devinfo_t, KEY_TYPE_STRING, name, NULL),
	MAKE_KEY_INFO(js_devinfo_t, KEY_TYPE_STRING, date, NULL),
	MAKE_KEY_INFO(js_devinfo_t, KEY_TYPE_U32, bSntp, NULL),
	MAKE_KEY_INFO(js_devinfo_t, KEY_TYPE_U32, sntpInterval, NULL),
	MAKE_KEY_INFO(js_devinfo_t, KEY_TYPE_STRING, ntpServer, NULL),
	MAKE_KEY_INFO(js_devinfo_t, KEY_TYPE_U32, tz, NULL),
	MAKE_KEY_INFO(js_devinfo_t, KEY_TYPE_U32, bDST, NULL),

	MAKE_END_INFO()
};

#define webdevinfo_GENERAL	"webdevinfo operation command"
#define webdevinfo_DETAIL	\
	"\nUsage: webdevinfo OPERATION [PARAM]\n"\
	"Operations:\n"\
	"  list\r\t\tList the dev info\n"\
	"  set\r\t\tSet the dev info with PARAM\n"\
	"  settime\r\t\tSet the dev time. webdevinfo settime 2012-06-07 13:58:00\n"\
	"  system\r\t\tSystem control, PARAM just:[reboot/reset/softreset]\n"\
	"\n"\
	"PARAM:\n"\
	"  PARAM is setted when OPTION is \"set\"\n"\
	"  You can use cmd: 'webdevinfo list' to see the format of PARAM\n"\
	"Example:\n"\
	"  webdevinfo set '{\"name\":\"ipcam-1\"}'\n"\
	"\n"

int webcmd_devinfo(int argc, char *argv[])
{
	int i,j;
	ipcinfo_t ipcinfo;
	js_devinfo_t devinfo, *mp;
	char *out;
	//用户鉴权，参数多两个
	//if (argc < 2)
	if (argc < ((CheckPower == 1)?4:2))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("param error");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[1],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}

	if (strcmp(argv[1], "list") == 0)
	{
		int ystCnt = 0;
		ipcinfo_get_param(&ipcinfo);

		//ipcinfo 2 devinfo fixed
		strcpy(devinfo.type, "ipc");
		strcpy(devinfo.hardware, ipcinfo.product);
		strcpy(devinfo.firmware, ipcinfo.version);
		strcpy(devinfo.manufacture, ipcinfo.product);
		sprintf(devinfo.model, "%s-module", devinfo.type);
		jv_ystNum_parse(devinfo.sn,  ipcinfo.nDeviceInfo[6],ipcinfo.ystID);
		devinfo.channelCnt = 1;
		devinfo.streamCnt = 3;
		for (i=0;i<devinfo.streamCnt;i++)
		{
			for (j=0;j<devinfo.channelCnt;j++)
			{
				devinfo.ystChannelNo[ystCnt++] = i*devinfo.channelCnt+j+1;
			}
		}

		//ipcinfo 2 devinfo not fixed
		strcpy(devinfo.name, ipcinfo.acDevName);
		strcpy(devinfo.date, ipcinfo.date);
		devinfo.bSntp = ipcinfo.bSntp;
		strcpy(devinfo.ntpServer, ipcinfo.ntpServer);
		devinfo.sntpInterval = ipcinfo.sntpInterval;
		devinfo.tz = ipcinfo.tz;
		devinfo.bDST = ipcinfo.bDST;

		out = cjson_object2string(json_devinfo,&devinfo);
		if (out != NULL)
		{
			weber_set_result(out);
			free(out);
			return 0;
		}
		else
		{
			weber_set_result("unknown error");
			return -1;
		}
	}
	else if (strcmp(argv[1], "set") == 0)
	{
		//用户鉴权，参数多两个
		//if (argc != 3)
		if (argc != ((CheckPower == 1)?5:3))
		{
			Printf("test argc: %d, param:%s\n", argc,argv[3]);
			weber_set_result("param error");
			return -1;
		}
		ipcinfo_get_param(&ipcinfo);
		mp = cjson_string2object(json_devinfo, argv[2], &devinfo);
		if (mp == NULL)
		{
			weber_set_result("param error");
			return -1;
		}
		strncpy(ipcinfo.acDevName, devinfo.name, sizeof(ipcinfo.acDevName));
		strcpy(ipcinfo.date, devinfo.date);
		ipcinfo.bSntp = devinfo.bSntp;
		strcpy(ipcinfo.ntpServer, devinfo.ntpServer);
		ipcinfo.sntpInterval = devinfo.sntpInterval;
		ipcinfo.tz = devinfo.tz;
		ipcinfo.bDST = devinfo.bDST;

		ipcinfo_set_param(&ipcinfo);
		weber_set_result("ok");
		_webcmd_save();
	}
	else if (strcmp(argv[1], "settime") == 0)
	{
		//用户鉴权，参数多两个
		//if (argc != 3)
		if (argc != ((CheckPower == 1)?5:3))
		{
			Printf("test argc: %d, param:%s\n", argc,argv[3]);
			weber_set_result("param error");
			return -1;
		}

		ipcinfo_set_time(argv[2]);

		weber_set_result("ok");
		_webcmd_save();
	}
	else if (strcmp(argv[1], "system") == 0)
	{
		printf("running...%d\n", __LINE__);
		//用户鉴权，参数多两个
		//if (argc != 3)
		if (argc != ((CheckPower == 1)?5:3))
		{
			Printf("test argc: %d, param:%s\n", argc,argv[3]);
			weber_set_result("param error");
			return -1;
		}

		if (strcmp(argv[2], "reboot") == 0)
		{
			//SYSFuncs_reboot();
			sp_dev_reboot(0);
		}
		else if (strcmp(argv[2], "reset") == 0)
		{
			//SYSFuncs_factory_default(1);
			sp_dev_factory_default(1);
		}
		else if (strcmp(argv[2], "softreset") == 0)
		{
			//SYSFuncs_factory_default(0);
			sp_dev_factory_default(0);
		}
		else
		{
			weber_set_result("param error");
			return -1;
		}
		printf("running...%d\n", __LINE__);

		weber_set_result("ok");
	}
	else
	{
		Printf("cmd not reco....\n");
	}
	return 0;
}
#endif
/* ISP 命令by liuyl 20130625*/

typedef enum
{
	PTZ_LENS_IRIS,
	PTZ_LENS_FOCUS,
	PTZ_LENS_ZOOM,
	PTZ_LENS_MAX,
}PTZ_LENS_TYPE_E;

typedef struct __PTZMOVE
{
	int chnid;
	float x;
	float y;
	float s;
}PTZMOVE;
typedef struct __PTZLENS
{
	int chnid;
	PTZ_LENS_TYPE_E type;
	int value;
}PTZLENS;

typedef enum
{
	PTZ_PRESET_LIST,
	PTZ_PRESET_ADD,
	PTZ_PRESET_DELL,
	PTZ_PRESET_START,
	PTZ_PRESET_MAX,
}PTZ_PRESET_TYPE_E;

typedef struct __PTZPRESET
{
	PTZ_PRESET_TYPE_E type;
	int chnid;
	int presetid;
	char name[32];
}WEBPTZPRESET;

typedef enum
{
	PTZ_PATROL_LIST,
	PTZ_PATROL_ADD,
	PTZ_PATROL_DELL,
	PTZ_PATROL_START,
	PTZ_PATROL_STOP,
	PTZ_PATROL_MAX,
}PTZ_PATROL_TYPE_E;

typedef struct __PTZPATROL
{
	PTZ_PATROL_TYPE_E type;
	int chnid;
	int patrolid;
	int presetid;
	int staytime;
}WEBPTZPATROL;

int webPtzLens(int chnid,PTZ_LENS_TYPE_E type,int value)
{
	printf("web lens param:%d,%d,%d\n",chnid,type,value);
	if(value==0)
	{
		weber_set_result("web ptz lens stop all!\n");
		PtzIrisOpenStop(chnid);
		PtzIrisCloseStop(chnid);
		PtzFocusNearStop(chnid);
		PtzFocusFarStop(chnid);
		PtzZoomOutStop(chnid);
		PtzZoomInStop(chnid);
		return 0;
	}
	else if(value!=1 && value!=-1)
	{
		weber_set_result("web ptz lens param error!\n");
		return -1;
	}
	switch(type)
	{
	case PTZ_LENS_IRIS:
		printf("web ptz lens iris:%s!\n",value==1?"+":"-");
		if(value==1)
			PtzIrisOpenStart(chnid);
		else if(value==-1)
			PtzIrisCloseStart(chnid);
		break;
	case PTZ_LENS_FOCUS:
		printf("web ptz lens FOCUS:%s!\n",value==1?"+":"-");
		if(value==1)
			PtzFocusNearStart(chnid);
		else if(value==-1)
			PtzFocusFarStart(chnid);
		break;
	case PTZ_LENS_ZOOM:
		printf("web ptz lens zoom:%s!\n",value==1?"+":"-");
		if(value==1)
			PtzZoomInStart(chnid);
		else if(value==-1)
			PtzZoomOutStart(chnid);
		break;
	default:
		return -1;
		break;
	}
	return 0;
}

int webPtzMove(int chn,float x,float y)
{
	BOOL bLeft,bUp;
	printf("web ptz move param:chnid:%d,x:%f,y:%f\n",chn,x,y);
	if(x>1||x<-1||y>1||y<-1)
	{
		printf("web ptz move param error\n");
		return -1;
	}
	int leftSpeed,upSpeed;
	if(x<0)
	{
		bLeft = TRUE;
		leftSpeed = (int)(255.0*(-x));
	}
	else
	{
		bLeft = FALSE;
		leftSpeed = (int)(255.0*x);

	}
	if(y>0)
	{
		bUp = TRUE;
		upSpeed=(int)(y*255.0);
	}
	else
	{
		bUp = FALSE;
		upSpeed=(int)((-y)*255.0);
	}

	if(x==0.0&&y==0.0)
	{
		printf("web ptz move stop\n");
		PtzPanTiltStop(chn);
	}
	else
	{
		printf("web ptz move LR:%s:%d,UD:%s:%d\n",bLeft?"L":"R",leftSpeed,bUp?"U":"D",upSpeed);
		PtzPanTiltStart(chn,bLeft,bUp,leftSpeed,upSpeed);
	}
	return 0;
}

int webPtzMoveAuto(int chn,float s)
{
	static BOOL bStartOrStop=0;//0: 没有在巡航,1:在巡航中 
	printf("web ptz auto move param:%d,%f\n",chn,s);
	if(s>1||s<0)
	{
		printf("web ptz auto move param error\n");
		return -1;
	}
	if(s!=0.0)
	{
		if(bStartOrStop == 0)
		{
			printf("web ptz auto move start:chn:%d,speed:%d\n",chn,(int)(s*255.0));
			PtzAutoStart(chn,(int)(s*255.0));
			bStartOrStop=1;
		}
		else
		{
			printf("web ptz auto move stop\n");
			PtzAutoStop(chn);
			bStartOrStop=0;
		}
	}
	else
	{
		//printf("web ptz auto move stop\n");
		//PtzAutoStop(chn);
	}
	return 0;
}

int webPtzPreset(WEBPTZPRESET *preseter)
{
	int i,ret = 0;
	PTZ_PRESET_INFO *presetinfo;
	char presetlist[MAX_PRESET_CT*128];
//	char name_gb2312[32] = {0};
	presetinfo = PTZ_GetPreset();
	switch(preseter->type)
	{
	case PTZ_PRESET_LIST:
		sprintf(presetlist,"{\"status\":\"ok\",\"data\":[");
		for(i = 0;i<presetinfo[0].nPresetCt;i++)
		{
			char tmp[100];
			sprintf(tmp,"{\"id\": %d,\"name\":\"%s\"},",presetinfo[preseter->chnid].pPreset[i],presetinfo[preseter->chnid].name[i]);
			strcat(presetlist,tmp);
		}
		if(presetlist[strlen(presetlist)-1] == ',')
			presetlist[strlen(presetlist)-1] = '\0';
		strcat(presetlist,"]}");
		printf("presetlist:%s\n",presetlist);
		weber_set_result(presetlist);
		return 0;
		break;
	case PTZ_PRESET_ADD:
		//utl_iconv_utf8togb2312(preseter->name,name_gb2312,32);
		//printf("utf8:%s,gb2312:%s\n",preseter->name,name_gb2312);
		ret = PTZ_AddPreset(preseter->chnid, preseter->presetid, preseter->name);
		if(ret == -1)
			weber_set_result("{\"status\":\"preset illegal\",\"data\":\"\"}");
		else if(ret == -2)
			weber_set_result("{\"status\":\"preset existed\",\"data\":\"\"}");
		else if(ret == -3)
			weber_set_result("{\"status\":\"preset count full\",\"data\":\"\"}");
		break;
	case PTZ_PRESET_DELL:
		ret = PTZ_DelPreset(preseter->chnid, preseter->presetid);
		if(ret == -1)
			weber_set_result("{\"status\":\"preset not exist\",\"data\":\"\"}");
		break;
	case PTZ_PRESET_START:
		PtzLocatePreset(preseter->chnid, preseter->presetid);
		break;
	default:
		return -1;
		break;
	}
	if(ret == 0)
		weber_set_result("{\"status\":\"ok\",\"data\":\"\"}");
	return ret;
}

char *GetPresetName(PTZ_PRESET_INFO *preset,U32 nPreset)
{
	U32 j;

	for (j=0;j<preset->nPresetCt;j++)
	{
		if (preset->pPreset[j] == nPreset)
		{
			return preset->name[j];
		}
	}
	return "";
}

int CheckPresetNum(PTZ_PRESET_INFO *preset,U32 nPreset)
{
	U32 j;

	for (j=0;j<preset->nPresetCt;j++)
	{
		if (preset->pPreset[j] == nPreset)
		{
			return 0;
		}
	}
	return -1;
}

int webPtzPatrol(WEBPTZPATROL *patroler)
{
	if(patroler->patrolid >= 1)
	{
		weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
		return -1;
	}
	U32 i,ret = 0;
	PTZ_PATROL_INFO *patrolinfo;
	PTZ_PRESET_INFO *presetinfo;
	char patrollist[MAX_PRESET_CT*32];
	patrolinfo = PTZ_GetPatrol();
	presetinfo = PTZ_GetPreset();
	switch(patroler->type)
	{
	case PTZ_PATROL_LIST:
		sprintf(patrollist,"{\"status\":\"ok\",\"data\":[");
		for(i = 0;i<patrolinfo[0].nPatrolSize;i++)
		{
			char tmp[100];
			char *name = GetPresetName(&presetinfo[patroler->chnid],patrolinfo[patroler->chnid].aPatrol[i].nPreset);
			sprintf(tmp,"{\"id\":%d,\"presetid\": %d,\"name\":\"%s\",\"staytime\":%d},",i,patrolinfo[patroler->chnid].aPatrol[i].nPreset,name,patrolinfo[patroler->chnid].aPatrol[i].nStayTime);
			strcat(patrollist,tmp);
		}
		if(patrollist[strlen(patrollist)-1] == ',')
			patrollist[strlen(patrollist)-1] = '\0';
		strcat(patrollist,"]}");
		weber_set_result(patrollist);
		return 0;
		break;
	case PTZ_PATROL_ADD:
		if(CheckPresetNum(&presetinfo[patroler->chnid],patroler->presetid)>=0)
		{
			ret = AddPatrolNod(&patrolinfo[patroler->patrolid],patroler->presetid,patroler->staytime);
			if(ret == -1)
				weber_set_result("{\"status\":\"preset count full\",\"data\":\"\"}");
		}
		else
		{
			ret = -1;
			weber_set_result("{\"status\":\"preset not exist\",\"data\":\"\"}");
		}
		break;
	case PTZ_PATROL_DELL:
		ret = DelPatrolNod(&patrolinfo[patroler->patrolid],patroler->presetid);
		if(ret == -1)
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
		break;
	case PTZ_PATROL_START:
		PTZ_StartPatrol(patroler->chnid, 0);
		break;
	case PTZ_PATROL_STOP:
		PTZ_StopPatrol(patroler->chnid);
		break;
	default:
		return -1;
	}
	if(ret == 0)
		weber_set_result("{\"status\":\"ok\",\"data\":\"\"}");
	return ret;
}

static json_kinfo_t json_nPtzLens[]=
{
	MAKE_KEY_INFO(PTZLENS, KEY_TYPE_U32   , type , NULL),
	MAKE_KEY_INFO(PTZLENS, KEY_TYPE_U32   , value, NULL),

	MAKE_END_INFO()
};

static json_kinfo_t json_nPtzMove[]=
{
	MAKE_KEY_INFO(PTZMOVE, KEY_TYPE_FLOAT, x	, NULL),
	MAKE_KEY_INFO(PTZMOVE, KEY_TYPE_FLOAT, y	, NULL),
	MAKE_KEY_INFO(PTZMOVE, KEY_TYPE_FLOAT, s	, NULL),

	MAKE_END_INFO()
};

static json_kinfo_t json_nPtzPreset[]=
{
	MAKE_KEY_INFO(WEBPTZPRESET, KEY_TYPE_U32, type		, NULL),
	MAKE_KEY_INFO(WEBPTZPRESET, KEY_TYPE_U32, chnid	, NULL),
	MAKE_KEY_INFO(WEBPTZPRESET, KEY_TYPE_U32, presetid	, NULL),
	MAKE_KEY_INFO(WEBPTZPRESET, KEY_TYPE_STRING, name	, NULL),

	MAKE_END_INFO()
};

static json_kinfo_t json_nPtzPatrol[]=
{
	MAKE_KEY_INFO(WEBPTZPATROL, KEY_TYPE_U32, type	, NULL),
	MAKE_KEY_INFO(WEBPTZPATROL, KEY_TYPE_U32, chnid	, NULL),
	MAKE_KEY_INFO(WEBPTZPATROL, KEY_TYPE_U32, patrolid	, NULL),
	MAKE_KEY_INFO(WEBPTZPATROL, KEY_TYPE_U32, presetid	, NULL),
	MAKE_KEY_INFO(WEBPTZPATROL, KEY_TYPE_U32, staytime	, NULL),

	MAKE_END_INFO()
};


#define webptz_GENERAL	"ptz operation command"
#define webptz_DETAIL	\
	"\nUsage: ptz OPERATION [PARAM]\n"\
	"\n"
int webcmd_ptz(int argc, char *argv[])
{
	PTZMOVE mover;
			mover.chnid = 0;
			mover.s = 0;
			mover.x = 0;
			mover.y = 0;
	void *mp;
	//if(_webcmd_checkpower(argv[argc-2],argv[argc-1])<0)
	//	return -1;
	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[1],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}
	
	if (strcmp(argv[1], "move") == 0)
	{
		if (argc < 3 || argc >5)
		{
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
		mp = cjson_string2object(json_nPtzMove, argv[2], &mover);
		if (mp == NULL)
		{
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
					return -1;
		}
		if(webPtzMove(mover.chnid, mover.x ,mover.y)<0)
		{
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
	}
	else if (strcmp(argv[1], "move_auto") == 0)
	{
		if (argc < 3 || argc >5)
		{
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
		mp = cjson_string2object(json_nPtzMove, argv[2], &mover);
		if (mp == NULL)
		{
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
		;
		if(webPtzMoveAuto(mover.chnid, mover.s)<0)
		{
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
	}
	else if (strcmp(argv[1], "lens") == 0)
	{
		PTZLENS lens;
		lens.chnid = 0;
		lens.type = 0;
		lens.value = 0;
		if (argc < 3 || argc >5)
		{
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
		mp = cjson_string2object(json_nPtzLens, argv[2], &lens);
		lens.chnid = 0;
		if (mp == NULL)
		{
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
		if(webPtzLens(lens.chnid,lens.type,lens.value)<0)
		{
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
	}
	else if (strcmp(argv[1], "preset") == 0)
	{
		WEBPTZPRESET preseter = {0};
		if (argc < 3)
		{
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
	//	printf("strlen:%d\n",strlen(argv[2]));
		if(strlen(argv[2])>73)
		{
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
		mp = cjson_string2object(json_nPtzPreset, argv[2], &preseter);
		preseter.chnid = 0;
		if (mp == NULL)
		{
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
		webPtzPreset(&preseter);
		_webcmd_save();
		return 0;
	}
	else if (strcmp(argv[1], "patrol") == 0)
	{
		WEBPTZPATROL patroler = {0};
		if (argc < 3)
		{
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
		mp = cjson_string2object(json_nPtzPatrol, argv[2], &patroler);
		patroler.chnid = 0;
		if (mp == NULL)
		{
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
		webPtzPatrol(&patroler);
		_webcmd_save();
		return 0;
	}
	weber_set_result("{\"status\":\"ok\",\"data\":\"\"}");
	return 0;
}


typedef enum
{
	MULTIMEDIA_IMAGE_CONTRAST,//对比度
	MULTIMEDIA_IMAGE_BRIGHTNESS,//亮度
	MULTIMEDIA_IMAGE_SATURATION,//饱和度
	MULTIMEDIA_IMAGE_SHARPNESS,//锐度
	MULTIMEDIA_IMAGE_MAX,
}MULTIMEDIA_IMAGE_TYPE_E;

typedef struct __MULTIMEDIA_IMAGE
{
	int chnid;
	MULTIMEDIA_IMAGE_TYPE_E type;
	float value;
}MULTIMEDIA_IMAGE;
static json_kinfo_t json_nMultimediaImage[]=
{
		MAKE_KEY_INFO(MULTIMEDIA_IMAGE, KEY_TYPE_U32   , chnid , NULL),
	MAKE_KEY_INFO(MULTIMEDIA_IMAGE, KEY_TYPE_U32   , type , NULL),
	MAKE_KEY_INFO(MULTIMEDIA_IMAGE, KEY_TYPE_FLOAT   , value, NULL),

	MAKE_END_INFO()
};

int multimediaImageSet(int chnid,MULTIMEDIA_IMAGE_TYPE_E type,float value)
{
	if(type > MULTIMEDIA_IMAGE_MAX)
		return -1;
	if(value<0||value>1)
		return -1;
	int nvalue = (int)(255.000*value);
	msensor_attr_t sensor;
	msensor_getparam(&sensor);
	printf("set type:%d,value:%d\n",type,nvalue);
	switch(type)
	{
	case MULTIMEDIA_IMAGE_CONTRAST:
		sensor.contrast = nvalue;
		break;
	case MULTIMEDIA_IMAGE_BRIGHTNESS:
		sensor.brightness = nvalue;
		break;
	case MULTIMEDIA_IMAGE_SATURATION:
		sensor.saturation = nvalue;
		break;
	case MULTIMEDIA_IMAGE_SHARPNESS:
		sensor.sharpness = nvalue;
		break;
	default:
		break;
	}
	msensor_setparam(&sensor);
	msensor_flush(0);
	_webcmd_save();
	//msensor_flush(0);
	printf("success\n");
	return 0;
}

float multimediaImageGet(int chnid,MULTIMEDIA_IMAGE_TYPE_E type)
{
	if(type > MULTIMEDIA_IMAGE_MAX)
		return -1;
	int result;
	msensor_attr_t sensor;
	msensor_getparam(&sensor);
	printf("get type:%d\n",type);
	switch(type)
	{
	case MULTIMEDIA_IMAGE_CONTRAST:
		result = sensor.contrast;
		break;
	case MULTIMEDIA_IMAGE_BRIGHTNESS:
		result = sensor.brightness;
		break;
	case MULTIMEDIA_IMAGE_SATURATION:
		result = sensor.saturation;
		break;
	case MULTIMEDIA_IMAGE_SHARPNESS:
		result = sensor.sharpness;
		break;
	default:
		result = 125;
		break;
	}
	printf("get value:%f\n",(float)result/255);
	return ((float)result)/255;
}

#define webmultimedia_GENERAL	"multimedia operation command"
#define webmultimedia_DETAIL	\
	"\nUsage: multimedia OPERATION [PARAM]\n"\
	"\n"
int webcmd_multimedia(int argc, char *argv[])
{
	MULTIMEDIA_IMAGE image;
	image.chnid = 0;
	image.type = 0;
	image.value = 0.5;
	void *mp;

	if(argc < 3 || argc > 5)
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[1],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}
	
	if (strcmp(argv[1], "imageset") == 0)
	{
		/*
		int ret = maccount_check_power_ex(argv[argc-2],argv[argc-1]);
		printf("account power:%d\n",ret);
		if(ret==ERR_NOTEXIST_EX || ret==ERR_PASSWD_EX)
		{
//			printf("username or password error\n");
			weber_set_result("{\"status\":\"username or password error\",\"data\":\"\"}");
			return -1;
		}
		if(ret==POWER_GUEST || ret == (POWER_GUEST|POWER_FIXED))
		{
			weber_set_result("{\"status\":\"without permission\",\"data\":\"\"}");
			return -1;
		}

		if (argc < 3 || argc >5)
		{
//			printf("param error1\n");
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
		*/
		mp = cjson_string2object(json_nMultimediaImage, argv[2], &image);
		if (mp == NULL)
		{
//			printf("param error2\n");
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
					return -1;
		}
		if(multimediaImageSet(image.chnid, image.type ,image.value)<0)
		{
//			printf("param error3\n");
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
		else
		{
			char str[64];
			sprintf(str,"{\"status\":\"ok\",\"data\":\"\"}");
			weber_set_result(str);
		}
	}
	if (strcmp(argv[1], "imageget") == 0)
	{
		/*
//		printf("webcmd_multimedia imageget..............running\n");
		int ret = maccount_check_power_ex(argv[argc-2],argv[argc-1]);
		if(ret==ERR_NOTEXIST_EX || ret==ERR_PASSWD_EX)
		{
//			printf("username or password error\n");
			weber_set_result("{\"status\":\"username or password error\",\"data\":\"\"}");
			return -1;
		}
		if (argc < 3 || argc >5)
		{
//			printf("param error4\n");
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
		*/
		mp = cjson_string2object(json_nMultimediaImage, argv[2], &image);
		if (mp == NULL)
		{
//			printf("param error5\n");
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
		float result = multimediaImageGet(image.chnid, image.type);
		if(result < 0)
		{
//			printf("param error6\n");
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}
		else
		{
			char str[64];
			sprintf(str,"{\"status\":\"ok\",\"data\":{\"value\":%f}}",result);
			weber_set_result(str);
		}
	}
	return 0;
}

#define websystem_GENERAL	"websystem operation command"
#define websystem_DETAIL	\
	"\nUsage: websystem OPERATION [PARAM]\n"\
	"\n"
int webcmd_system(int argc, char *argv[])
{
	//参数个数校验，加入用户鉴权，个数为4
	if (argc != ((CheckPower == 1)?4:2))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[1],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}

	if (strcmp(argv[1], "reboot") == 0)
	{
		//1、统一用户鉴权接口，且提到检验前面
		//2、修改参数个数校验，参数个数确定为4个
		/*int ret = maccount_check_power_ex(argv[argc-2],argv[argc-1]);
		printf("account_power:%d\n",ret);
		if(ret==ERR_NOTEXIST_EX || ret==ERR_PASSWD_EX)
		{
			weber_set_result("{\"status\":\"username or password error\",\"data\":\"\"}");
			return -1;
		}
		if(ret==POWER_GUEST || ret == POWER_USER || ret == (POWER_GUEST|POWER_FIXED))
		{
			weber_set_result("{\"status\":\"without permission\",\"data\":\"\"}");
			return -1;
		}
		if (argc < 2 || argc >4)
		{
			weber_set_result("{\"status\":\"param error\",\"data\":\"\"}");
			return -1;
		}*/

		//utl_system("reboot");
		SYSFuncs_reboot();
	}
	weber_set_result("{\"status\":\"ok\",\"data\":\" \"}");
	return 0;
}

static json_kinfo_t json_audio[] =
{
	//MAKE_KEY_INFO(SPAudioAttr_t, KEY_TYPE_U32, sampleRate		, NULL),
	//MAKE_KEY_INFO(SPAudioAttr_t, KEY_TYPE_U32, bitWidth			, NULL),
	//MAKE_KEY_INFO(SPAudioAttr_t, KEY_TYPE_U32, encType			, NULL),
	MAKE_KEY_INFO(jv_audio_attr_t, KEY_TYPE_U32, sampleRate		, NULL),
	MAKE_KEY_INFO(jv_audio_attr_t, KEY_TYPE_U32, bitWidth		, NULL),
	MAKE_KEY_INFO(jv_audio_attr_t, KEY_TYPE_U32, encType			, NULL),
	MAKE_KEY_INFO(jv_audio_attr_t, KEY_TYPE_U32, level			, NULL),
	MAKE_KEY_INFO(jv_audio_attr_t, KEY_TYPE_U32, micGain			, NULL),
	MAKE_END_INFO()
};


#define webaudio_GENERAL	"webaudio operation command"
#define webaudio_DETAIL	\
	"\nUsage: webaudio OPERATION [PARAM]\n"\
	"\n"
int webcmd_audio(int argc, char *argv[])
{
	printf("webaudio action : %s \n",argv[1]);
	char *out;
	int ret;
	jv_audio_attr_t attr,*mp;

	//if (argc < 2)
	if (argc < ((CheckPower == 1)?4:2))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("param error");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[1],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}

	memset(&attr,0,sizeof(jv_audio_attr_t));
	jv_ai_get_attr(0,&attr);
	if (strcmp(argv[1], "list") == 0)
	{
		out = cjson_object2string(json_audio,&attr);
		if (out != NULL)
		{
			weber_set_result(out);
			free(out);
			return 0;
		}
		else
		{
			weber_set_result("unknown error");
			return -1;
		}
	}
	else if (strcmp(argv[1], "set") == 0)
	{
		//if (argc != 3)
		if (argc != ((CheckPower == 1)?5:3))
		{
			printf("the number of params isn't right , argc=%d\n",argc);
			weber_set_result("param error");
			return -1;
		}
		mp = cjson_string2object(json_audio, argv[2], &attr);
		if (mp == NULL)
		{
			weber_set_result("param error");
			return -1;
		}
		mstream_audio_set_param(0,&attr);
		weber_set_result("ok");
		_webcmd_save();
	}
	
	return 0;
}


static int _jv_redirect(int argc, char *argv[])
{

	char buf[128];
	char result[100];

	memset(result,'\0',100);
	//if (argc < 2)
	if (argc < ((CheckPower == 1)?4:2))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		//设置返回报文
		weber_set_result("Bad param");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[1],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}
	
	if (strcmp(argv[1], "to") == 0)
	{
		//if (argc < 3)
		if (argc < ((CheckPower == 1)?5:3))
		{
			printf("the number of params isn't right , argc=%d\n",argc);
			//设置返回报文
			weber_set_result("Bad param");
			return -1;
		}
		strcpy(buf,argv[2]);
		freopen(buf,"w",stdout);
		freopen(buf,"w",stderr);
		printf("Current tty: %s\n",buf);
		//设置返回报文
		sprintf(result,"{\"status\":\"ok\",\"Current tty\":\"%s\"}",buf);
		weber_set_result(result);
	}
	else if(strcmp(argv[1], "resume") == 0)
	{
		freopen("/dev/console","w",stdout);
		freopen(buf,"w",stderr);
		printf("Current tty: %s\n",buf);
		//设置返回报文
		sprintf(result,"{\"status\":\"ok\",\"Current tty\":\"%s\"}",buf);
		weber_set_result(result);
	}

	return 0;
}

typedef struct
{
	char hour;
	char minute;
}_dayStart,_dayEnd;

static json_kinfo_t json_dayStart[] =
{
	MAKE_KEY_INFO(_dayStart, KEY_TYPE_U8, hour				, NULL),
	MAKE_KEY_INFO(_dayStart, KEY_TYPE_U8, minute				, NULL),

	MAKE_END_INFO()
};

static json_kinfo_t json_image_ajust[] =
{
	MAKE_KEY_INFO(SPImage_t, KEY_TYPE_U32, contrast			, NULL),  //对比度 0-255
	MAKE_KEY_INFO(SPImage_t, KEY_TYPE_U32, brightness		, NULL),  //亮度 0-255
	MAKE_KEY_INFO(SPImage_t, KEY_TYPE_U32, saturation		, NULL),  //饱和度0-255
	MAKE_KEY_INFO(SPImage_t, KEY_TYPE_U32, sharpen			, NULL),  //锐度0-255
	MAKE_KEY_INFO(SPImage_t, KEY_TYPE_U32, exposureMax		, NULL),  //最大曝光时间。曝光时间为 ： 1/exposureMax 秒，取值 3 - 100000
	MAKE_KEY_INFO(SPImage_t, KEY_TYPE_U32, exposureMin		, NULL),
	MAKE_KEY_INFO(SPImage_t, KEY_TYPE_U32, scene			, NULL),  //场景
	MAKE_KEY_INFO(SPImage_t, KEY_TYPE_U32, daynightMode		, NULL),  //日夜模式
	MAKE_ARRAY_INFO(SPImage_t, KEY_TYPE_ARRAY, dayStart	, json_dayStart, 1, KEY_TYPE_OBJECT),
	MAKE_ARRAY_INFO(SPImage_t, KEY_TYPE_ARRAY, dayEnd	, json_dayStart, 1, KEY_TYPE_OBJECT),
	MAKE_KEY_INFO(SPImage_t, KEY_TYPE_U32, bEnableAWB		, NULL),  //是否自动白平衡..auto white balance
	MAKE_KEY_INFO(SPImage_t, KEY_TYPE_U32, bEnableMI		, NULL),  //是否画面镜像..mirror image
	MAKE_KEY_INFO(SPImage_t, KEY_TYPE_U32, bEnableST		, NULL),  //是否画面翻转..screen turn
	MAKE_KEY_INFO(SPImage_t, KEY_TYPE_U32, bEnableNoC		, NULL),  //是否黑白模式..no color
	MAKE_KEY_INFO(SPImage_t, KEY_TYPE_U32, bEnableWDynamic	, NULL),  //是否开启宽动态..wide dynamic
	MAKE_KEY_INFO(SPImage_t, KEY_TYPE_U32, bNightOptimization, NULL),  //是否夜视优化
	MAKE_KEY_INFO(SPImage_t, KEY_TYPE_U32, bAutoLowFrameEn	, NULL),  //是否夜视自动降帧
	
	MAKE_END_INFO()
};

int webcmd_image_adjust(int argc, char *argv[])
{
	SPImage_t image,*mp;
	char *out;

	//参数个数合法性校验
	if (argc < ((CheckPower == 1)?4:2))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("param error");
		return -1;
	}
	
	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[1],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}
	
	memset(&image,0,sizeof(SPImage_t));
	//先get一遍数据，list时直接输出，set时可防止缺省数据被覆盖
	sp_image_get_param(&image);

	if (strcmp(argv[1], "list") == 0)
	{
		//参数个数合法性校验，加上用户权限校验，个数为4
		if (argc != ((CheckPower == 1)?4:2))
		{
			printf("list params are not enough \n");
			weber_set_result("param error");
			return -1;
		}
		out = cjson_object2string(json_image_ajust,&image);
		if (out != NULL)
		{
			weber_set_result(out);
			free(out);
		}
		else
		{
			printf("json for list return failed \n");
			weber_set_result("json error");
			return -1;
		}
	}
	else if (strcmp(argv[1], "set") == 0)
	{
		//加入用户鉴权，参数为5个
		if (argc != ((CheckPower == 1)?5:3))
		{
			printf("set params are not enough \n");
			weber_set_result("param error");
			return -1;
		}
		mp = cjson_string2object(json_image_ajust, argv[2], &image);
		if (mp == NULL)
		{
			printf("json for set return failed \n");
			weber_set_result("param error");
			return -1;
		}
		
		if(-1 == sp_image_set_param(&image))  //返回-1表示参数校验不通过
		{
			printf("set params are illegal \n");
			weber_set_result("param error");
			return -1;
		}
		weber_set_result("ok");
	}
	return 0;
}

int webcmd_led(int argc, char *argv[])
{
	char result[100];
	int status;
	IOLedType_e type = IO_LED_BUTT;
	char strType[IO_LED_BUTT][20] = {"LED_RED",
									 "LED_GREEN",
									 "LED_BLUE",
									 "LED_INFRARED"};
	
	memset(result,'\0',100);
	if (argc < ((CheckPower == 1)?4:2))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("Bad param");
		return -1;
	}

	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[1],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}
	
	if (strcmp(argv[1], "list") == 0)
	{
		char tmp[50] = {0};
		for(type = IO_LED_RED ; type <= IO_LED_BLUE ; type++)
		{
			//status = jv_get_led(type);
			status = (status > 0)?(0):((status == 0)?(1):(status));
			sprintf(tmp,"%s status : %d\n",strType[type],status);
			strcat(result,tmp);
		}
		weber_set_result(result);
	}
	else if(strcmp(argv[1], "set") == 0)
	{
		if (argc < ((CheckPower == 1)?5:3))
		{
			printf("the number of params isn't right , argc=%d\n",argc);
			weber_set_result("Bad param for set");
			return -1;
		}

		status = (atoi(argv[2]) == 0)?(IO_OFF):(IO_ON);
		for(type = IO_LED_RED ; type <= IO_LED_BLUE ; type++)
		{
			//jv_set_led(status);
		}
		
		weber_set_result("ok");
	}

	return 0;
}

int webcmd_adetect(int argc, char *argv[])
{
	return 0;
}

int valid_time;
U32 telnetd_get_code(char *code)
{
	//取出ipcam的SN
	ipcinfo_t ipc_info;
	ipcinfo_get_param(&ipc_info);
	U32 nSN = ipc_info.nDeviceInfo[4];
	char ver[20] = {};
	char type[32] = {};
	unsigned int tmpver = 0;
	char tmptype[12];
	int i;
	
	for(i = 0;i < strlen(ipc_info.version);i++)
	{
		if(ipc_info.version[i] >= '0' && ipc_info.version[i] <= '9')
		{
			strncat(ver,&ipc_info.version[i],1);
		}
	}
	sscanf(ver,"%d",&tmpver);
	
	U32 nTime = (U32)time(NULL);
	nTime = nTime / (valid_time);		//临时密码
	unsigned int nCode = nSN + nTime + nTime * 2313;
	for(i = 0;i < strlen(ipc_info.type);i++)
	{
		if(ipc_info.type[i] != '-')
		{
			sprintf(tmptype,"%X",(ipc_info.type[i])^(nCode % 10));
			strncat(type,tmptype,2);
		}
	}
	sprintf(code,"%XI%c%XV%XT%s",nCode,ipc_info.nDeviceInfo[6],ipc_info.ystID^(nCode % 100),tmpver^(nCode % 100),type);
	return 0;//(nSN + nTime + nTime * 2313);
}
U32 telnetd_get_tmppasswd()
{
	U32 nCode, nPW, nSN;
	U32 nTime = (U32)time(NULL);
	if(valid_time <= 0)
		return -1;
	nTime = nTime / (valid_time);		//临时密码
	//取出ipcam的SN
	nSN = ipcinfo_get_param(NULL)->nDeviceInfo[4];

	nCode = nSN + nTime + nTime * 2313;

	nPW = (nCode+(nCode*13)+(nCode%17))^55296;
	return nPW;
}
int webcmd_getcode(int argc, char *argv[])
{
	if (argc < ((CheckPower == 1)?5:2))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("Bad param");
		return -1;
	}
	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[2],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}
	char codebuf[128] = {};
	char code[128];
	sscanf(argv[1],"%d",&valid_time);
	if(valid_time <= 0)
	{	
		weber_set_result("Bad param : param must be greater than zero ");
		return -1;
	}
	telnetd_get_code(code);
	sprintf(codebuf,"code: %s",code);
	weber_set_result(codebuf);
	return 0;
}


int webcmd_telnetd(int argc, char *argv[])
{
	if (argc < ((CheckPower == 1)?5:3))
	{
		printf("the number of params isn't right , argc=%d\n",argc);
		weber_set_result("Bad param");
		return -1;
	}
	//用户鉴权
	if(0 != _webcmd_checkpower(argv[0],argv[2],argv[argc-2],argv[argc-1]))
	{
		printf("user power check failed \n");
		return -1;
	}
	
	if (strcmp(argv[1], "start") == 0)
	{
		U32 tmpcode;
		sscanf(argv[2],"%x",&tmpcode);
		if(tmpcode == telnetd_get_tmppasswd())
		{
			utl_system("telnetd");
			weber_set_result("OK");
		}
		else
		{
			weber_set_result("Verification code error");
		}
	}
	return 0;
}



int webcmd_init(void)
{
	CheckPower = 0;

	utl_cmd_insert("account",webaccount_GENERAL,webaccount_DETAIL,webcmd_account);
	utl_cmd_insert("webalarm",webalarm_GENERAL,webalarm_DETAIL,webcmd_alarm);
	utl_cmd_insert("webmdetect",webmdetect_GENERAL,webmdetect_DETAIL,webcmd_mdetect);
	utl_cmd_insert("webprivacy",webprivacy_GENERAL,webprivacy_DETAIL,webcmd_privacy);
	utl_cmd_insert("webrecord",webrecord_GENERAL,webrecord_DETAIL,webcmd_record);
	utl_cmd_insert("webstorage",webstorage_GENERAL,webstorage_DETAIL,webcmd_storage);
	utl_cmd_insert("webstream",webstream_GENERAL,webstream_DETAIL,webcmd_stream);
	utl_cmd_insert("webifconfig",webifconfig_GENERAL,webifconfig_DETAIL,webcmd_ifconfig);
	utl_cmd_insert("webwifi",webwifi_GENERAL,webwifi_DETAIL,webcmd_wifi);
	utl_cmd_insert("webosd",webosd_GENERAL,webosd_DETAIL,webcmd_osd);
	utl_cmd_insert("websnapshot",websnapshot_GENERAL,websnapshot_DETAIL,webcmd_snapshot);
	utl_cmd_insert("webhelp",webhelp_GENERAL,webhelp_DETAIL,webcmd_help);
	utl_cmd_insert("webipcinfo",webipcinfo_GENERAL,webipcinfo_DETAIL,webcmd_ipcinfo);
	utl_cmd_insert("webdevinfo",webdevinfo_GENERAL,webdevinfo_DETAIL,webcmd_devinfo);
	//原有的webexpose,webwhite,webgamma,websharpen,webccm不再使用，统一换成webimage,调用sp接口
	utl_cmd_insert("webimage","image operation command","image data list & set",webcmd_image_adjust);
    utl_cmd_insert("yst",webyst_GENERAL,webyst_DETAIL,webcmd_yst);
    utl_cmd_insert("system",websystem_GENERAL,websystem_DETAIL,webcmd_system);
    utl_cmd_insert("multimedia",webmultimedia_GENERAL,webmultimedia_DETAIL,webcmd_multimedia);
    utl_cmd_insert("ptz",webptz_GENERAL,webptz_DETAIL,webcmd_ptz);
	utl_cmd_insert("webaudio",webaudio_GENERAL,webaudio_DETAIL,webcmd_audio);
	utl_cmd_insert("redirect", "redirect stdout stderror", "redirect to xx;redirect close", _jv_redirect);
	utl_cmd_insert("webled", "led control command", "led on/off list&set", webcmd_led);
	utl_cmd_insert("webad", "audio detect", "audio detect list&set", webcmd_adetect);
	utl_cmd_insert("telnetd", "telnetd start", "telnetd start get check code&start", webcmd_telnetd);
	utl_cmd_insert("getcode", "get code", "get check code", webcmd_getcode);

	return 0;
}

