/*
 * sp_user.c
 *
 *  Created on: 2013-11-18
 *      Author: Administrator
 */
#include <jv_common.h>
#include "sp_user.h"
#include "maccount.h"

static ACCOUNT *__psuser2account(ACCOUNT *act, SPUser_t *user)
{
	memset(act, 0, sizeof(ACCOUNT));

	strncpy(act->acID, user->name, 20);
	strncpy(act->acPW, user->passwd, 20);
	strncpy(act->acDescript, user->descript, 32);
	switch (user->level)
	{
	case PS_USER_LEVEL_Administrator:
		act->nPower |= POWER_ADMIN;
		break;
	case PS_USER_LEVEL_Operator:
		act->nPower |= POWER_USER;
		break;
	case PS_USER_LEVEL_User:
		act->nPower |= POWER_GUEST;
		break;
	default:
		act->nPower |= POWER_GUEST;
		break;
	}

	return act;
}

static SPUser_t *__account2psuser(SPUser_t *user, ACCOUNT *act)
{
	memset(user, 0, sizeof(SPUser_t));

	strncpy(user->name, act->acID, 20);
	strncpy(user->passwd, act->acPW, 20);
	strncpy(user->descript, act->acDescript, 32);
	if (act->nPower & POWER_ADMIN)
		user->level = PS_USER_LEVEL_Administrator;
	else if (act->nPower & POWER_USER)
		user->level = PS_USER_LEVEL_Operator;
	else if (act->nPower & POWER_GUEST)
		user->level = PS_USER_LEVEL_User;

	return user;
}

/**
 * @brief 获取用户总数
 *
 * return 用户总数
 */
int sp_user_get_cnt(void)
{
	return maccount_get_cnt();
}

/**
 * @brief 根据用户编号获取用户信息
 *
 * @param index 用户编号
 * @param user 返回用户信息
 *
 * @return 0 表示成功
 */
int sp_user_get(int index, SPUser_t *user)
{
	ACCOUNT *muser = maccount_get(index);
	__account2psuser(user, muser);

	return 0;
}

/**
 * @brief 根据用户名获取用户编号
 *
 * @param 用户名
 *
 * @return 用户编号
 */
int sp_user_get_index(const char *name)
{
	int i;
	SPUser_t user;
	if (!name)
		return -1;
	int cnt = sp_user_get_cnt();
	for (i = 0; i < cnt; i++)
	{
		sp_user_get(i, &user);
		if (strcmp(user.name, name) == 0)
		{
			return i;
		}
	}

	//用户不存在
	return -1;
}

/**
 * @brief 修改用户信息，注：用户名一旦注册不可修改，但可删除
 *
 * @param user 修改后的用户信息，用户名为原用户名
 *
 * return 0 成功
 */
int sp_user_set(SPUser_t *user)
{
	ACCOUNT muser;
	int ret = 0;
	__psuser2account(&muser, user);
	ret = maccount_modify(&muser);
	return ret;
}

/**
 * @brief 删除用户信息
 *
 * @param user 要删除的用户信息
 *
 * @return 0 成功
 */
int sp_user_del(SPUser_t *user)
{
	ACCOUNT muser;
	int ret = 0;
	__psuser2account(&muser, user);
	ret = maccount_remove(&muser);
	return ret;
}

/**
 * @brief 添加用户
 *
 * @param user 要添加的用户信息
 *
 * @return 0 成功
 */
int sp_user_add(SPUser_t *user)
{
	ACCOUNT muser;
	int ret = 0;
	__psuser2account(&muser, user);
	ret = maccount_add(&muser);
	return ret;
}

/**
 * @brief 验证用户密码权限
 *
 * @param 用户名
 * @param 用户密码
 *
 * @return -1 验证失败
 * 		   >=0 用户权限值，参照：SPUserLevel_e
 */
SPUserLevel_e sp_user_check_power(char *name, char *passwd)
{
	int power = maccount_check_power(name, passwd);
	switch(power)
	{
	case POWER_ADMIN:
		return PS_USER_LEVEL_Administrator;
		break;
	case POWER_ADMIN | POWER_FIXED:
		return PS_USER_LEVEL_Administrator;
		break;
	case POWER_USER:
		return PS_USER_LEVEL_Operator;
		break;
	case POWER_GUEST:
		return PS_USER_LEVEL_User;
		break;
	case POWER_GUEST | POWER_FIXED:
		return PS_USER_LEVEL_User;
		break;
	default:
		return -1;
		break;
	}
	return -1;
}
