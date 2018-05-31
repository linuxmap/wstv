#include "jv_common.h"
#include "maccount.h"
#include "mipcinfo.h"
#include "mlog.h"
#include "mstream.h"
#include "mbizclient.h"
#include "sgrpc.h"
#include "utl_aes.h"

#define SECRET_KEY		0x1053564A

#define ACCOUNT_FILE	CONFIG_PATH"/account.dat"
#define	MAX_ACCOUNT		8

//默认密码。如果与默认密码一致，则不验证密码
#define DEFAULT_PASSWD	""

static ACCOUNT	stAccount[MAX_ACCOUNT];


static S32 _maccount_save();

/**
 *@brief 在语言变化时，修正帐户中的描述
 *@param nLanguage 它的定义，与mipcinfo.h中定义相同，取值LANGUAGE_EN或LANGUAGE_CN
 *
 */
void maccount_fix_with_language(int nLanguage)
{
	int i;
	
	for(i=0; i<MAX_ACCOUNT; i++)
	{
		if (stAccount[i].nIndex != -1)
		{
//			Printf("nIndex:%d, ID:%s, PW:%s\n", stAccount[i].nIndex, stAccount[i].acID, stAccount[i].acPW);
			if (strcmp(stAccount[i].acID, "guest") == 0)
			{
				if (nLanguage == LANGUAGE_EN)
				{
					strcpy(stAccount[i].acDescript, "Account for Guest");
				}
				else
				{
					strcpy(stAccount[i].acDescript, "来访帐户");
				}
			}
			else if (strcmp(stAccount[i].acID, "admin") == 0)
			{
				if (nLanguage == LANGUAGE_EN)
				{
					strcpy(stAccount[i].acDescript, "Account for Administrator");
				}
				else
				{
					strcpy(stAccount[i].acDescript, "管理员帐户");
				}
			}
		}
	}
//	Printf("xian stAccount set over\n");
	//修改后立刻保存
	_maccount_save();
}

static int __maccount_read()
{
	FILE *fIn;
	char *buffer;
	int len;
	int i;
	char sum;

	fIn=fopen(ACCOUNT_FILE, "rb");

	if (!fIn)
	{
		return -1;
	}

	fseek(fIn, 0, SEEK_END);
	len = ftell(fIn);
	fseek(fIn, 0, SEEK_SET);
	buffer = malloc(len);
	if (!buffer)
	{
		Printf("ERROR: Failed malloc memory\n");
		fclose(fIn);
		return -1;
	}

	fread(buffer, 1, len, fIn);
	sum = 0;
	for (i=0;i<len-1;i++)
	{
		sum += buffer[i];
	}
	if (sum == buffer[i])
	{
	}
	else
	{
		/*printf("sum check failed: %d, %d\n", sum, buffer[i]);*/
		free(buffer); 
		buffer = NULL;
		fclose(fIn);
		return -1;
	}
	memcpy(stAccount, buffer, sizeof(stAccount));

	U32 *pData	= (U32*)stAccount;
	for(i=0; i<sizeof(stAccount)/sizeof(U32); i++)
	{
		pData[i] ^= SECRET_KEY;
	}
	free(buffer); 
	buffer = NULL;
	fclose(fIn);
	return 0;
}

S32 maccount_init(void)
{
	U32	i;

	//初始化
	for(i=0; i<MAX_ACCOUNT; i++)
	{
		stAccount[i].nIndex = -1;
	}

	//在文件中加载用户信息
	if(0 != __maccount_read())
	{
		//默认账户,abc
		i = 0;
		stAccount[i].nIndex	= i;
		stAccount[i].nPower	= POWER_ADMIN|POWER_FIXED;
		strcpy(stAccount[i].acID, "admin");
		strcpy(stAccount[i].acPW, DEFAULT_PASSWD);
		strcpy(stAccount[i].acDescript, "");//取消默认的描述信息。为了多语言时好显示

#ifdef BIZ_CLIENT_SUPPORT
		mbizclient_updateuserinfo_server(stAccount[i].acID,stAccount[i].acPW);
#endif

		maccount_fix_with_language(ipcinfo_get_param(NULL)->nLanguage);
		return 0;
	}

	return 0;
}

static S32 _maccount_save()
{
	U32	i, *pData=NULL;
	FILE *fOut = NULL;
	ACCOUNT list[MAX_ACCOUNT+1];
	char sum;
	char *buffer;
	int len;

	fOut=fopen(ACCOUNT_FILE, "wb");
	if(!fOut)
	{
		Printf("Failed open account file: %s, err: %s\n", ACCOUNT_FILE, strerror(errno));
		return -1;
	}
	memcpy(list, stAccount, sizeof(stAccount));
	pData	= (U32*)list;
	for(i=0; i<sizeof(list)/sizeof(U32); i++)
	{
		pData[i] ^= SECRET_KEY;
	}
	fwrite(list, 1, sizeof(list), fOut);
	sum = 0;
	len = sizeof(list);
	buffer = (char *)list;
	for (i=0;i<len;i++)
	{
		sum += buffer[i];
	}
	fwrite(&sum, 1, 1, fOut);
	fclose(fOut);

	//保存时，顺便修改一下RTSP的密码
	mstream_rtsp_user_changed();
	sgrpc_account_refresh();
	return 0;
}

//增加帐号
S32 maccount_add(PACCOUNT pstAccount)
{
	U32 i;
	const char *p = pstAccount->acID;
	while(p && *p)
	{
		if (*p == ';')
			return ERR_EXISTED;
		p++;
	}
	char *pDes = pstAccount->acDescript;
	while(pDes && *pDes)
	{
		if (*pDes == ';')
			*pDes = ',';
		pDes++;
	}
	for(i=0; i<MAX_ACCOUNT; i++)
	{
		if(-1 == stAccount[i].nIndex)
		{
			memcpy(&stAccount[i], pstAccount, sizeof(ACCOUNT));
			stAccount[i].nIndex = i;
			_maccount_save();
			mlog_write("Account Add: [%s] Success", pstAccount->acID);
			return 0;
		}
		else
		{
			if(!strcmp(stAccount[i].acID, pstAccount->acID))
			{
				mlog_write("Account Add: [%s] Existed", pstAccount->acID);
				return ERR_EXISTED;
			}
		}
	}
	mlog_write("Account Add: [%s] Limited", pstAccount->acID);
	return ERR_LIMITED;
}

//更新帐号
S32 maccount_modify(PACCOUNT pstAccount)
{
	U32 i;
	for(i=0; i<MAX_ACCOUNT; i++)
	{
		if(stAccount[i].nIndex >= 0 && 0 == strcmp(stAccount[i].acID, pstAccount->acID))
		{
			memcpy(&stAccount[i], pstAccount, sizeof(ACCOUNT));

			//第一个账户，即admin添加上FIXED权限
			if(0 == i)
			{
				stAccount[i].nPower |= POWER_FIXED;
			}
			stAccount[i].nIndex = i;
			_maccount_save();
#ifdef BIZ_CLIENT_SUPPORT
			mbizclient_updateuserinfo_server(stAccount[i].acID,stAccount[i].acPW);
#endif
			mlog_write("Account Modify: [%s] Success", pstAccount->acID);
			return 0;
		}
	}
	mlog_write("Account Modify: [%s] Not Exist", pstAccount->acID);
	return ERR_NOTEXIST;
}

S32 maccount_modifypw(char* acID,char *oldpwd,char* newpwd)
{
	U32 i;
	for(i=0; i<MAX_ACCOUNT; i++)
	{
		if(stAccount[i].nIndex >= 0 && 0 == strcmp(stAccount[i].acID, acID))
		{
			if(0 == strcmp(stAccount[i].acPW, oldpwd))
			{
				strncpy(stAccount[i].acPW,newpwd,sizeof(stAccount[i].acPW));
				//第一个账户，即admin添加上FIXED权限
				if(0 == i)
				{
					stAccount[i].nPower |= POWER_FIXED;
				}
				stAccount[i].nIndex = i;
				_maccount_save();
				mlog_write("Account Modify: [%s] Success", acID);
				return 0;
			}
			else
				return ERR_PASSWD;
		}
	}
	mlog_write("Account Modify: [%s] Not Exist", acID);
	return ERR_NOTEXIST;
}

//更新列表
static VOID _maccount_updatelist()
{
	U32 i, j;
	ACCOUNT stTmp[MAX_ACCOUNT];	//不用初始化

	for(i=0,j=0; i<MAX_ACCOUNT; i++)
	{
	    if(stAccount[i].nIndex > MAX_ACCOUNT || stAccount[i].nIndex < -1)// 消除未知 填充的account结构体
		{
            memset(&stAccount[i], 0, sizeof(ACCOUNT));
			stAccount[i].nIndex	= -1;
		}
		stTmp[i].nIndex = -1;
		if(-1 != stAccount[i].nIndex)
		{
			memcpy(&stTmp[j], &stAccount[i], sizeof(ACCOUNT));
			j++;
		}
	}
	memcpy(stAccount, stTmp, sizeof(stTmp));
}

//删除帐号
S32 maccount_remove(PACCOUNT pstAccount)
{
	U32 i;
	for(i=0; i<MAX_ACCOUNT; i++)
	{
		if(stAccount[i].nIndex < 0)
		{
			break;
		}
		//找到要删除的ID,删除
		if(!strcmp(stAccount[i].acID, pstAccount->acID))
		{
			if (stAccount[i].nPower & POWER_FIXED)
			{
				mlog_write("Account Remove: [%s] Not Permision", pstAccount->acID);
				return ERR_PERMISION_DENIED;
			}
			
			memset(&stAccount[i], 0, sizeof(ACCOUNT));
			stAccount[i].nIndex	= -1;
			Printf("DelAccount: id=%s, pw=%s\n", pstAccount->acID, pstAccount->acPW);
			_maccount_updatelist();
			_maccount_save();
			mlog_write("Account Remove: [%s] Success", pstAccount->acID);
			return 0;
		}
	}
	mlog_write("Account Remove: [%s] Not Exist", pstAccount->acID);
	return ERR_NOTEXIST;
}

/**
 *@brief check if need check passwd.
 *@retval 0 need to check power
 *@retval otherwise, power of admin
 */
BOOL maccount_need_check_power(void)
{
	ACCOUNT *pUser;
	pUser = &stAccount[0];
	if (0 == strcmp(pUser->acID, "admin") && strcmp(pUser->acPW, DEFAULT_PASSWD) == 0)
	{
		return FALSE;
	}
	return TRUE;
}

/**
 *@brief check power of account
 *@param id id of account
 *@param passwd password of the account
 *@retval 0 id and passwd invalid
 *@retval >0 power of the id
 *
 */
U32 maccount_check_power(const char *id, const char *passwd)
{
	int i;
	ACCOUNT *pUser;
	U32 ret;
	if (!maccount_need_check_power())
	{
		return POWER_ADMIN|POWER_FIXED;
	}
	
	for(i=0; i<MAX_ACCOUNT; i++)
	{
		pUser= &stAccount[i];
		if (pUser->nIndex != -1)
		{
			if(strcmp(id, pUser->acID) == 0 && strcmp(passwd, pUser->acPW) == 0)
			{
				return pUser->nPower;
			}
		}
	}

	return 0;
}

/**
 *@brief check power of account
 *@param id id of account
 *@param passwd password of the account
 *@retval 0 id and passwd invalid
 *@retval >0 power of the id
 *
 */
U32 maccount_check_power_ex(const char *id, const char *passwd)
{
	int i;
	ACCOUNT *pUser;
	U32 ret;
	if (!maccount_need_check_power())
	{
		return POWER_ADMIN|POWER_FIXED;
	}

	for(i=0; i<MAX_ACCOUNT; i++)
	{
		pUser= &stAccount[i];
		if (pUser->nIndex != -1)
		{
			if(strcmp(id, pUser->acID) == 0)
			{
				if(strcmp(passwd, pUser->acPW) == 0)
				{
					return pUser->nPower;
				}
				else
					return ERR_PASSWD_EX;
			}
		}
	}

	if(i==MAX_ACCOUNT)
		return ERR_NOTEXIST_EX;

	return 0;
}

int maccount_get_cnt(void)
{
	int i;

	_maccount_updatelist();
	for (i=0;i<MAX_ACCOUNT;i++)
		if (stAccount[i].nIndex == -1)
			break;
	return i;
}

ACCOUNT *maccount_get(int index)
{
	_maccount_updatelist();
	return &stAccount[index];
}

U32 maccount_get_errorcode()
{
	//取出ipcam的SN
	U32 nSN = ipcinfo_get_param(NULL)->nDeviceInfo[4];
	U32 nTime = (U32)time(NULL);
	nTime = nTime / (3600 * 72);		//临时密码三天内有效

	return (nSN + nTime + nTime * 2313);
}

U32 maccount_get_tmppasswd()
{
	U32 nErr, nPW, nSN;
	U32 nTime = (U32)time(NULL);
	nTime = nTime / (3600 * 72);		//临时密码三天内有效

	//取出ipcam的SN
	nSN = ipcinfo_get_param(NULL)->nDeviceInfo[4];

	nErr = nSN + nTime + nTime * 2313;

	nPW = (nErr+(nErr*13)+(nErr%17))^55296;

	return nPW;
}

