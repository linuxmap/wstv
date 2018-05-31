#ifndef UTL_IPSCAN_H_
#define UTL_IPSCAN_H_

//lfx20130824
//此文件提供的功能用于得到一个空闲的IP地址

/**
 *@brief 扫描指定的IP地址列表中的内容，检查其是否被占用
 *
 *@param addrList 要扫描的IP地址列表，网络字节序
 *@param cnt 要扫描的IP地址列表的长度
 *@param timeoutMS 超时时间，单位为毫秒。时间越长，结果越准确
 *@param bUsed 输出。记录扫描到的IP地址的有效性。为真则表示该地址有效
 *
 *@return 0 成功， -1 失败
 */
int utl_ipscan(unsigned int *addrList, int cnt, unsigned int timeoutMS, int *bUsed);


/**
 *@brief 扫描指定的IP地址所在的网段，获取IP地址被占用的列表
 *
 *@param ipstr 作为参考的IP地址
 *@param timeoutMS 超时时间，单位为毫秒。时间越长，结果越准确
 *@param ipList IP地址被占用的列表，格式为网络字节序。
 *@param maxCnt 意指ipList能容纳的最大IP地址的数目
 *
 *@return 扫描到的IP地址的个数, -1 失败
 */
int utl_ipscan_local(const char *ipstr, unsigned int timeoutMS, unsigned int *ipList, int maxCnt);

/**
 *@brief 扫描指定的IP地址所在的网段，获取IP地址未被占用的列表
 *
 *@param ipstr 作为参考的IP地址
 *@param timeoutMS 超时时间，单位为毫秒。时间越长，结果越准确
 *@param ipList IP地址空闲的列表，格式为网络字节序。
 *@param maxCnt 意指ipList能容纳的最大IP地址的数目
 *
 *@return 扫描到的IP地址的个数, -1 失败
 */
int utl_ipscan_not_used(const char *ipstr, unsigned int timeoutMS, unsigned int *ipList, int maxCnt);

/**
 *@param ipstr 本地IP地址。作为参考使用，形如192.168.11.35
 *@param timeoutMS 超时时间。时间越长，结果越准确
 *
 *@return 空闲的IP地址。网络字节序的UINT值 . 0 表示未找到，-1表示失败了
 *
 */
unsigned int utl_get_free_ip(const char *ipstr, unsigned int timeoutMS);

#endif

