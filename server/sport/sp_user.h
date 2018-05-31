/*
 * sp_user.h
 *
 *  Created on: 2013-11-18
 *      Author: LK
 */

#ifndef SP_USER_H_
#define SP_USER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 这里要稍等注意一下
 * 中维的用户名等级与ONVIF稍有差别，它们的对应关系如下：
 * USER_LEVEL_Administrator == POWER_ADMIN
 * USER_LEVEL_Operator == POWER_USER
 * USER_LEVEL_USER == POWER_GUEST
 */
typedef enum
{
	PS_USER_LEVEL_Administrator,
	PS_USER_LEVEL_Operator,
	PS_USER_LEVEL_User,
	PS_USER_LEVEL_Anonymous,
	PS_USER_LEVEL_Extended,
	PS_USER_LEVEL_MAX
}SPUserLevel_e;

#define	ERR_USER_EXISTED		0x1	///< 用户已存在
#define	ERR_USER_LIMITED		0x2	///< 用户太多，超出了限制
#define	ERR_USER_NOTEXIST	0x3	///< 指定的用户不存在
#define	ERR_USER_PASSWD		0x4	///< 密码错误
#define	ERR_USER_PERMISION_DENIED	0x5

typedef struct
{
	char name[32];				//用户名
	char passwd[32];			//密码
	SPUserLevel_e level;		//权限分组
	char descript[32];			//说明描述
}SPUser_t;

/**
 * @brief 获取用户总数
 *
 * return 用户总数
 */
int sp_user_get_cnt(void);

/**
 * @brief 根据用户编号获取用户信息
 *
 * @param index 用户编号
 * @param user 返回用户信息
 *
 * @return 0 表示成功
 */
int sp_user_get(int index, SPUser_t *user);

/**
 * @brief 根据用户名获取用户编号
 *
 * @param 用户名
 *
 * @return 用户编号
 */
int sp_user_get_index(const char *name);

/**
 * @brief 修改用户信息，注：用户名一旦注册不可修改，但可删除
 *
 * @param user 修改后的用户信息，用户名为原用户名
 *
 * return 0 成功
 */
int sp_user_set(SPUser_t *user);

/**
 * @brief 删除用户信息
 *
 * @param user 要删除的用户信息
 *
 * @return 0 成功
 */
int sp_user_del(SPUser_t *user);

/**
 * @brief 添加用户
 *
 * @param user 要添加的用户信息
 *
 * @return 0 成功
 */
int sp_user_add(SPUser_t *user);

/**
 * @brief 验证用户密码权限
 *
 * @param 用户名
 * @param 用户密码
 *
 * @return 0 验证失败
 * 		   >0 用户权限值，参照：SPUserLevel_e
 */
SPUserLevel_e sp_user_check_power(char *name, char *passwd);

#ifdef __cplusplus
}
#endif


#endif /* SP_USER_H_ */
