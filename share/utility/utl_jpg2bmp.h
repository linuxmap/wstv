/*
 * utl_jpg2bmp.h
 *
 *  Created on: 2015-09-06
 *      Author: zwq
 */

#ifndef UTL_JPG2BMP_H_
#define UTL_JPG2BMP_H_

#ifdef __cplusplus
extern "C"
{
#endif

/************************************
提供码流从jpg转换到bmp格式的功能
input:	pJpgData,jpg码流字符串，带必要的消息头
		jpgLen,指示jpg格式码流的长度
output:	pBmpData,空间由调用申请释放，bmp格式的码流，带bmp消息头
		bmpLen,指示bmp格式码流的长度
return:	成功返回0，操作异常返回-1
************************************/
int _transformJpg2Bmp(unsigned char *pJpgData, int jpgLen,unsigned char *pBmpData, int *bmpLen);

#ifdef __cplusplus
}
#endif

#endif
