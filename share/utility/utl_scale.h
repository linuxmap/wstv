/*
 * utl_scale.h
 *
 *  Created on: 2013年12月31日
 *      Author: lfx  20451250@qq.com
 */
#ifndef UTL_SCALE_H_
#define UTL_SCALE_H_

typedef enum{
	UTL_SCALE_FMT_YUV420,
//	UTL_SCALE_FMT_YUV422,
}UtlScaleFmt_e;

typedef struct{
	UtlScaleFmt_e dataFmt;
	int bQuickly;	//快速算法

	int inw;
	int inh;
	int outw;
	int outh;
}UtlScalParam_t;

typedef void * SCALE_HANDLE;

/**
 *@brief 创建缩放器句柄
 */
SCALE_HANDLE utl_scale_create(UtlScalParam_t *param);

/**
 *@brief 缩放
 *
 *@param handle 缩放器句柄
 *@param inbuf 输入参数。其长度由UtlScalParam_t中的参数计算得出
 *@param outbuf 输出参数。其长度由UtlScalParam_t中的参数计算得出
 */
int utl_scale(SCALE_HANDLE handle, const unsigned char *inbuf, unsigned char *outbuf);

/**
 *@brief 销毁缩放器
 */
int utl_scale_destroy(SCALE_HANDLE handle);


int utl_yuv420_scaled(unsigned char *dstY, unsigned char *dstU, unsigned char *dstV,
		const unsigned char *srcY, const unsigned char *srcU, const unsigned char *srcV, int DstWidth,
	int DstHeight, int srcWidth, int srcHeight);

#endif /* UTL_YUVSCAL_H_ */
