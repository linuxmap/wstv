/**
 *@file 读取配置文件用的文件
 * 该文件的格式为，每行一个配置项，每个配置项写法：key=value
 * 前边不可以有"="，如果必须加"="，则需要使用\=转义
 *
 *
 */

#ifndef _UTL_FILECFG_H_
#define _UTL_FILECFG_H_

#ifndef SEEK_SET
#  define SEEK_SET        0       /* Seek from beginning of file.  */
#  define SEEK_CUR        1       /* Seek from current position.  */
#  define SEEK_END        2       /* Set file pointer to EOF plus "offset" */
#endif

typedef struct
{
	char *key;
	char *value;
}keyvalue_t;

/**
 *@brief 从指定文件中获取KEY值对应的value
 *@param fname 要读取的文件名
 *@param key 要查找的KEY值
 *
 *@return 查找到的KEY对应的值
 *
 */
char *utl_fcfg_get_value(const char *fname, const char *key);

/**
 *@brief 从指定文件中获取KEY值对应的VALUE，类型为INT
 */
int utl_fcfg_get_value_int(const char *fname, const char *key, unsigned int defval);

/**
 *@brief 从指定文件中获取KEY值对应的value
 *@param fname 要读取的文件名
 *@param key 要查找的KEY值
 *@param valuebuf 输出参数
 *@param maxlen #valuebuf 的长度
 *
 *@return 成功时为valuebuf，否则为Null
 *
 */
char *utl_fcfg_get_value_ex(const char *fname, const char *key, char *valuebuf, int maxlen);

/**
 *@brief 设置某个key的对应的值
 *@param fname 要设置的文件名
 *@param key 要设置的KEY值，这个key值，允许不存在
 *
 *@return 0 成功
 *
 */
int utl_fcfg_set_value(const char *fname, const char *key, char *value);

/**
 *@brief 准备getnext
 */
int utl_fcfg_start_getnext(const char *fname);

/**
 *@brief 结束getnext
 */
int utl_fcfg_end_getnext(const char *fname);

/**
 *@brief 从文件中获取下一个配置项
 * 用于轮询文件中所有配置项
 *
 *@param fname 要读取的文件名
 *@param current IN/OUT 初始值为0.用于记录当前取到哪一个了
 *
 *@return 返回下一个配置项
 *
 */
keyvalue_t *utl_fcfg_get_next(const char *fname, int *current);

/**
 *@brief 刷新信息到文件中
 *
 *@param fname 要刷新的文件名
 *@return 0 成功
 *
 */
int utl_fcfg_flush(const char *fname);

/**
 *@brief 清空文件缓存，当再也不再使用该文件时调用
 *
 *@param fname 要读取的文件名
 *
 *@note 当配置文件有改变时，可以调用此方法，直到刷新该文件的作用
 *
 *@return 0 成功
 */
int utl_fcfg_close(const char *fname);

#endif

