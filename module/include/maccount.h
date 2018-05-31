/**
 *@file maccount.h 帐户管理
 * Copyright (C) 2011 Jovision Technology Co., Ltd.
 * 此文件用来组织帐号管理相关代码，考虑到帐号模块比较独立，此模块的一些宏定义，结构体在文件内
 * 更改历史详见svn版本库日志
 */

#ifndef __MACCOUNT_H__
#define __MACCOUNT_H__

#define	SIZE_ID			20
#define	SIZE_PW			20
#define SIZE_DESCRIPT	32

/**
 *@brief 函数返回的错误号
 *
 */
#define	ERR_EXISTED		0x1	///< 用户已存在
#define	ERR_LIMITED		0x2	///< 用户太多，超出了限制
#define	ERR_NOTEXIST	0x3	///< 指定的用户不存在
#define	ERR_PASSWD		0x4	///< 密码错误
#define	ERR_PERMISION_DENIED	0x5

#define	ERR_EXISTED_EX			0x100	///< 用户已存在
#define	ERR_LIMITED_EX			0x200	///< 用户太多，超出了限制
#define	ERR_NOTEXIST_EX			0x300	///< 指定的用户不存在
#define	ERR_PASSWD_EX			0x400	///< 密码错误
#define	ERR_PERMISION_DENIED_EX	0x500

/**
 *@brief 用户组定义
 *
 */
#define POWER_GUEST		0x0001
#define POWER_USER		0x0002
#define POWER_ADMIN		0x0004
#define POWER_FIXED		0x0010

typedef struct tagACCOUNT
{
	S32		nIndex;			///< 0~MAX_ACCOUNT，-1表示没有使用
	char	acID[SIZE_ID];	///< ID注册后不可更改，但可以删除
	char	acPW[SIZE_PW];	///< 密码，可以更改
	char	acDescript[SIZE_DESCRIPT];///<账户描述
	U32		nPower;			///<权限，可更改
}ACCOUNT, *PACCOUNT;

/**
 *@brief 初始化
 *
 */
S32 maccount_init(void);

/**
 *@brief 添加一个帐户
 *@param pstAccount 新添加的帐户结构体指针
 *@return 成功时返回0， 否则返回#ERR_EXISTED 等值
 *
 */
S32 maccount_add(PACCOUNT pstAccount);

/**
 *@brief 修改帐户
 *@param pstAccount 新添加的帐户结构体指针
 *@return 成功时返回0， 否则返回#ERR_EXISTED 等值
 *
 */
S32 maccount_modify(PACCOUNT pstAccount);

/**
 *@brief 验证旧密码方式修改密码
 *@param oldpwd 旧密码
 *@param newpwd 新密码
 *@return 成功时返回0， 否则返回#ERR_EXISTED 等值
 */
S32 maccount_modifypw(char* acID,char *oldpwd,char* newpwd);

/**
 *@brief 修改帐户
 *@param pstAccount 要删除的帐户结构体指针
 *@return 成功时返回0， 否则返回#ERR_EXISTED 等值
 *
 */
S32 maccount_remove(PACCOUNT pstAccount);

/**
 *@brief check if need check passwd.
 *@retval 0 need to check power
 *@retval otherwise, power of admin
 */
BOOL maccount_need_check_power(void);

/**
 *@brief 获取用户权限
 *@param id 用户名
 *@param passwd 用户密码
 *@retval 0 用户名不存在或密码错误
 *@retval >0 用户权限值，请参考#POWER_GUEST等定义
 *
 */
U32 maccount_check_power(const char *id, const char *passwd);


/**
 *增加了返回值类型
 *
 */
U32 maccount_check_power_ex(const char *id, const char *passwd);
/**
 *@brief 获取帐户数量
 *@return 帐户数量
 *
 */
int maccount_get_cnt(void);

/**
 *@brief 获取帐户信息
 *@param index 帐户的序号
 *@return 帐户信息
 *
 */
ACCOUNT *maccount_get(int index);

/**
 *@brief 获取错误码
 *@param no
 *@return 错误码
 *
 */
U32 maccount_get_errorcode();

/**
 *@brief 获取临时密码
 *@param no
 *@return 临时生成的密码
 *
 */
U32 maccount_get_tmppasswd();

/**
 *@brief 在语言变化时，修正帐户中的描述
 *@param nLanguage 它的定义，与mipcinfo.h中定义相同，取值LANGUAGE_EN或LANGUAGE_CN
 *
 */
void maccount_fix_with_language(int nLanguage);

#endif
