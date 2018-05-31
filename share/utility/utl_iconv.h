#ifndef _UTL_ICONV_H_
#define _UTL_ICONV_H_

#ifdef __cplusplus
extern "C"
{
#endif
/**
 *@brief 检查字符串，检查其结束位是否正常
 * 在编码非法时，作为字符串的结束
 *
 *@param str 要检查的字符串
 *@param len 字符串允许的最大长度
 *
 */
int utl_iconv_gb2312_fix(char *str, int len);

/**
 *@brief 将GB2312转化为UTF8编码
 *@param src GB2312编码的字符串，也可能是英文或数字
 *@param des 编码后的输出
 *@param maxLen 编码后的最大长度
 *
 *@note des的长度，必须是src的1.5倍，才能保证其不越界
 *
 *@return 长度
 */
int utl_iconv_gb2312toutf8(char *src, char *des, int maxLen);
/*
 * utf8转gb2312
 * 注意des的长度至少为src的三分之二
 */
void utl_iconv_utf8togb2312(char *src, char *des, int maxLen);

#ifdef __cplusplus
}
#endif

#endif

