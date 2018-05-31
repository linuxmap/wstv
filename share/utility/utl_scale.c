/*
 * _jv_yuvscal.c
 *
 *  Created on: 2013年12月31日
 *      Author: lfx  20451250@qq.com
 */


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include "utl_scale.h"

#define SCALER_JOHN_2ND

#define SIMPLE_METHOD 1

typedef struct{
	UtlScalParam_t param;

	int len;
#if SIMPLE_METHOD
	unsigned char *tbuf;
#else
	unsigned char *yuva[3];
	unsigned char *yuvb[3];
	int pitch;	//宽度，注意单位为字节
	unsigned char *ptr;
#endif

}Scale_t;


#define PRESICION           16

#define ONE                 (1<<PRESICION)
#define HALF                (ONE>>1)
#define ZERO                0
#define FixedToInt(a)       (int) ((a)>>PRESICION)
#define ROUND(a)             FixedToInt((a)+HALF)

int utl_yuv420_scaled(unsigned char *dstY, unsigned char *dstU, unsigned char *dstV,
	const unsigned char *srcY, const unsigned char *srcU, const unsigned char *srcV, int DstWidth,
	int DstHeight, int srcWidth, int srcHeight)

{
	int i = 0, j = 0;
	int row = 0, column = 0, line = 0;
	int srcPos = 0, srcPos0 = 0, srcUVPos0 = 0, srcUVPos = 0, dstPos = 0, dstPosUV =
			0;
	int scaleFactorX = 0, scaleFactorY = 0;
	int tmp_src_line = 0, tmp_src_column = 0;
	int k = 0;
	unsigned long *dstY32 = NULL, Yvalue = 0, Uvalue = 0, Vvalue = 0;
	unsigned short *dstU16 = NULL, *dstV16 = NULL;

	if (DstWidth == 0 || DstHeight == 0) {
		return -1;
	}

	if (srcWidth % 2 != 0 || srcHeight % 2 != 0 || DstWidth % 2 != 0
			|| DstHeight % 2 != 0) {
		return -2;
	}

	scaleFactorX = (srcWidth << PRESICION) / DstWidth;
	scaleFactorY = (srcHeight << PRESICION) / DstHeight;

	if ((DstWidth & 0x03) == 0)  // dstwidth%4
			{
		dstY32 = (unsigned long *) dstY;
		dstU16 = (unsigned short *) dstU;
		dstV16 = (unsigned short *) dstV;
		for (j = 0; j < DstHeight; j++) {
			column = 0;
			tmp_src_column = 0;
			srcPos0 = line * srcWidth;
			srcUVPos0 = (line / 2) * (srcWidth / 2);
			dstPos = j * DstWidth / 4; // T_U32; dstPos=j*DstWidth;
			dstPosUV = (j / 2) * (DstWidth / 2) / 2; // T_U16; dstPosUV=(j/2)*(DstWidth/2);

			for (i = 0; (column < srcWidth) && (i < DstWidth); i += 4) {
				Yvalue = 0;
				Uvalue = 0;
				Vvalue = 0;
				for (k = 0; k < 4; k++) {
					srcPos = srcPos0 + column;

					Yvalue = (srcY[srcPos] << k * 8) | Yvalue;
					if ((j % 2 == 0) && (k % 2 == 0)) {
						srcUVPos = srcUVPos0 + (column / 2);
						Uvalue = (srcU[srcUVPos] << k / 2 * 8) | Uvalue;
						Vvalue = (srcV[srcUVPos] << k / 2 * 8) | Vvalue;
					}

					tmp_src_column += scaleFactorX;
					column = ROUND(tmp_src_column);
				}

				dstY32[dstPos++] = Yvalue;
				if (j % 2 == 0) {
					dstU16[dstPosUV] = Uvalue;
					dstV16[dstPosUV++] = Vvalue;
				}
			}    // for(i...)
			tmp_src_line += scaleFactorY;
			line = ROUND(tmp_src_line);

		}    // for(j...)

	} else {
		for (j = 0; j < DstHeight; j++) {
			column = 0;
			tmp_src_column = 0;
			srcPos0 = line * srcWidth;
			srcUVPos0 = (line / 2) * (srcWidth / 2);
			dstPos = j * DstWidth;
			dstPosUV = (j / 2) * (DstWidth / 2);

			for (i = 0; (column < srcWidth) && i < DstWidth; i++) {
				srcPos = srcPos0 + column;
				dstY[dstPos++] = srcY[srcPos];
				if ((j % 2 == 0) && (i % 2 == 0)) {
					srcUVPos = srcUVPos0 + (column / 2);
					dstU[dstPosUV] = srcU[srcUVPos];
					dstV[dstPosUV++] = srcV[srcUVPos];
				}

				tmp_src_column += scaleFactorX;
				column = ROUND(tmp_src_column);

			}    // for(i...)
			tmp_src_line += scaleFactorY;
			line = ROUND(tmp_src_line);

		}    // for(j...)

	}

	return 0;

}


int utl_yuv420_scaled11(unsigned char *dstY, unsigned char *dstU, unsigned char *dstV,
	const unsigned char *srcY, const unsigned char *srcU, const unsigned char *srcV, int DstWidth,
	int DstHeight, int srcWidth, int srcHeight)
{
	int i = 0, j = 0;
	int row = 0, column = 0, line = 0;
	int srcPos = 0, srcPos0 = 0, srcUVPos0 = 0, srcUVPos = 0, dstPos = 0, dstPosUV =
			0;
	int scaleFactorX = 0, scaleFactorY = 0;
	int tmp_src_line = 0, tmp_src_column = 0;

	if (DstWidth == 0 || DstHeight == 0) {
		return -1;
	}

	if (srcWidth % 2 != 0 || srcHeight % 2 != 0 || DstWidth % 2 != 0
			|| DstHeight % 2 != 0) {
		return -2;
	}

	scaleFactorX = (srcWidth << PRESICION) / DstWidth;
	scaleFactorY = (srcHeight << PRESICION) / DstHeight;
	for (j = 0; j < DstHeight; j++) {
		column = 0;
		tmp_src_column = 0;
		srcPos0 = line * srcWidth;
		srcUVPos0 = (line / 2) * (srcWidth / 2);
		dstPos = j * DstWidth;
		dstPosUV = (j / 2) * (DstWidth / 2);

		for (i = 0; (column < srcWidth); i++) {
			srcPos = srcPos0 + column;
			dstY[dstPos++] = srcY[srcPos];
			if (i % 2 == 0) {
				srcUVPos = srcUVPos0 + (column / 2);
				dstU[dstPosUV] = srcU[srcUVPos];
				dstV[dstPosUV++] = srcV[srcUVPos];
			}

			tmp_src_column += scaleFactorX;
			column = ROUND(tmp_src_column);

		}    // for(i...)
		tmp_src_line += scaleFactorY;
		line = ROUND(tmp_src_line);

	}    // for(j...)

	return 0;

}



/**
 *@brief 创建缩放器句柄
 */
SCALE_HANDLE utl_scale_create(UtlScalParam_t *param)
{
	int i;
	if (!param)
	{
		printf("ERROR: utl_scale_create bad param\n");
		return NULL;
	}
	Scale_t *scale = malloc(sizeof(Scale_t));

	memset(scale, 0, sizeof(Scale_t));
	scale->param = *param;


	scale->len = param->inw * param->inh;
	int temp = param->outw * param->outh;
	if (scale->len < temp)
		scale->len = temp;
#if SIMPLE_METHOD
//	scale->tbuf = malloc(scale->len);
#else
	scale->pitch = param->inw > param->outw ? param->inw : param->outw;
	scale->ptr = malloc(scale->len * 6);
	for (i=0;i<3;i++)
	{
		scale->yuva[i] = scale->ptr + (scale->len*i);
		scale->yuvb[i] = scale->ptr + (scale->len*(3+i));
	}
#endif
	return (SCALE_HANDLE)scale;
}

static int __utl_scale_one(int inw, int inh, int outw, int outh, const unsigned char *inbuf, unsigned char *outbuf, unsigned char *tbuf)
{
	int pic_width_in  = inw;
	int pic_height_in = inh;
	int pic_width_out  = outw;
	int pic_height_out = outh;

	int width_scaler_cof = pic_width_in * 256 / pic_width_out;
	int height_scaler_cof = pic_height_in * 256 / pic_height_out;

	int i,j,k;

	int width_scaler_cnt = 0;//水平缩放系数累加器，低8bit表示相位，其余bit表示目标像素在源图像中最靠近它的左边一个像素
	int height_scaler_cnt = 0;//垂直缩放系数累加器，低8bit表示相位，其余bit表示目标像素在源图像中最靠近它的上边一个像素


	// ============================================================
	// Read input YUV file data


	// Read Y data
	unsigned char *p = (unsigned char *)inbuf;
	int offset;

	// ============================================================
	// 水平方向缩放
	offset = 0;
	for( j=0; j<pic_height_in; j++ , offset += pic_width_in)
	{
		int left_pixel_data;
		int left_pixel_pos = 0;

		width_scaler_cnt = 0;

		// 预读取线性插值需要的两个像素中的左边一个像素
		left_pixel_data = inbuf[j*pic_width_in];//rgb_in_data[0][j][0];

		for( i=0; i<pic_width_out; i++ )
		{
			int pixel_pos = width_scaler_cnt >> 8;//目标点左边像素
			int h_scaler_dist_r = 256 - (width_scaler_cnt & 0xff);//目标点离右边像素的距离
			int h_scaler_dist_l = width_scaler_cnt - left_pixel_pos;//目标点离被选取去插值的左边像素的距离

			int right_pixel_data;

			{
				int pixel_out_temp;
				int h_scaler_coef_r, h_scaler_coef_l;

				right_pixel_data = inbuf[j*pic_width_in+pixel_pos+1];//rgb_in_data[pixel_pos+1][j][p];

				if( (h_scaler_dist_r + h_scaler_dist_l) & (1<<11) )
				{
					h_scaler_coef_r = ((h_scaler_dist_l & 0xff) >> 3) | 0xe0;
					h_scaler_coef_l = 256 - h_scaler_coef_r;
				}
				else if( (h_scaler_dist_r + h_scaler_dist_l) & (1<<10) )
				{
					h_scaler_coef_r = ((h_scaler_dist_l & 0xff) >> 2) | 0xc0;
					h_scaler_coef_l = 256 - h_scaler_coef_r;
				}
				else if( (h_scaler_dist_r + h_scaler_dist_l) & (1<<9) )
				{
					h_scaler_coef_r = ((h_scaler_dist_l & 0xff) >> 1) | 0x80;
					h_scaler_coef_l = 256 - h_scaler_coef_r;
				}
				else if( (h_scaler_dist_r + h_scaler_dist_l) & (1<<8) )
				{
					h_scaler_coef_r = h_scaler_dist_l;
					h_scaler_coef_l = 256 - h_scaler_coef_r;
				}
				else
				{
					h_scaler_coef_r = 0;
					h_scaler_coef_l = 0;
				}

				pixel_out_temp = (h_scaler_coef_r * right_pixel_data) + (h_scaler_coef_l * left_pixel_data);
				tbuf[j*pic_width_out+i] = (pixel_out_temp + 128) >> 8;//四舍五入
			}

			if( (width_scaler_cnt & ~0xff) != ((width_scaler_cnt + width_scaler_cof) & ~0xff) )
			{
				left_pixel_pos = (width_scaler_cnt & ~0xff) + 256;//当前插值的右边像素变成下次插值的左边像素
				left_pixel_data = inbuf[j*pic_width_in+pixel_pos+1];//rgb_in_data[0][j][0];
			}
			width_scaler_cnt += width_scaler_cof;//插值系数累加到下一个点
		}
	}


	// ============================================================
	// 垂直方向缩放
	for( i=0; i<pic_width_out; i++ )
	{
		int left_pixel_data;
		int left_pixel_pos = 0;

		width_scaler_cnt = 0;

		// 预读取线性插值需要的两个像素中的左边一个像素
		left_pixel_data = tbuf[i];

		for( j=0; j<pic_height_out; j++ )
		{
			int pixel_pos = width_scaler_cnt >> 8;//目标点左边像素
			int h_scaler_dist_r = 256 - (width_scaler_cnt & 0xff);//目标点离右边像素的距离
			int h_scaler_dist_l = width_scaler_cnt - left_pixel_pos;//目标点离被选取去插值的左边像素的距离

			int right_pixel_data;

			for( k=0; k<1; k++ )
			{
				int pixel_out_temp;
				int h_scaler_coef_r, h_scaler_coef_l;

				right_pixel_data = tbuf[(pixel_pos+1)*pic_width_out+i];//rgb_out_data[i][pixel_pos+1][p];

				if( (h_scaler_dist_r + h_scaler_dist_l) & (1<<11) )
				{
					h_scaler_coef_r = ((h_scaler_dist_l & 0xff) >> 3) | 0xe0;
					h_scaler_coef_l = 256 - h_scaler_coef_r;
				}
				else if( (h_scaler_dist_r + h_scaler_dist_l) & (1<<10) )
				{
					h_scaler_coef_r = ((h_scaler_dist_l & 0xff) >> 2) | 0xc0;
					h_scaler_coef_l = 256 - h_scaler_coef_r;
				}
				else if( (h_scaler_dist_r + h_scaler_dist_l) & (1<<9) )
				{
					h_scaler_coef_r = ((h_scaler_dist_l & 0xff) >> 1) | 0x80;
					h_scaler_coef_l = 256 - h_scaler_coef_r;
				}
				else if( (h_scaler_dist_r + h_scaler_dist_l) & (1<<8) )
				{
					h_scaler_coef_r = h_scaler_dist_l;
					h_scaler_coef_l = 256 - h_scaler_coef_r;
				}
				else
				{
					h_scaler_coef_r = 0;
					h_scaler_coef_l = 0;
				}

				pixel_out_temp = (h_scaler_coef_r * right_pixel_data) + (h_scaler_coef_l * left_pixel_data);
//				rgb_in_data[i][j][p] = (pixel_out_temp + 128) >> 8;//四舍五入
				outbuf[j*pic_width_out+i] = (pixel_out_temp + 128) >> 8;//四舍五入
			}

			if( (width_scaler_cnt & ~0xff) != ((width_scaler_cnt + height_scaler_cof) & ~0xff) )
			{
				left_pixel_pos = (width_scaler_cnt & ~0xff) + 256;//当前插值的右边像素变成下次插值的左边像素
				left_pixel_data = tbuf[(pixel_pos+1)*pic_width_out+i];//rgb_out_data[i][pixel_pos+1][0];//拷贝右边像素到左边像素位置，为下次插值做准备
			}
			width_scaler_cnt += height_scaler_cof;//插值系数累加到下一个点
		}
	}

	return 0;
}

static int __utl_scale_one_quickly(int inw, int inh, int outw, int outh, const unsigned char *inbuf, unsigned char *outbuf, unsigned char *tbuf)
{
	int i,j;
	int xscale = outw * 256 / inw;
	int yscale = outh * 256 / inh;

	unsigned char *src;
	unsigned char *dst;
	int x;
	for (i=0;i<outw;i++)
	{
		x = i *256 / xscale;
		src = (unsigned char *)inbuf + x;
		dst = tbuf + i;
		for (j=0;j<inh;j++)
		{
			*dst = *src;
			dst += outw;
			src += inw;
		}
	}

	dst = outbuf;
	for (j=0;j<outh;j++)
	{
		int y = j * 256 / yscale;
		src = tbuf + (y * outw);
		memcpy(dst, src, outw);
		dst += outw;
	}

	return 0;
}

#if SIMPLE_METHOD
int utl_scale(SCALE_HANDLE handle, const unsigned char *inbuf, unsigned char *outbuf)
{
	Scale_t *scale = (Scale_t *)handle;
	if (!handle)
	{
		printf("ERROR: scale destroy param error\n");
		return -1;
	}

	if (scale->param.bQuickly)
	{
		utl_yuv420_scaled(outbuf, outbuf+(scale->param.outw*scale->param.outh), outbuf+(scale->param.outw*scale->param.outh*5/4),
				inbuf, inbuf+(scale->param.inw*scale->param.inh), inbuf+(scale->param.inw*scale->param.inh*5/4),
				scale->param.outw, scale->param.outh,
				scale->param.inw, scale->param.inh);
	}else
	if (0)
	{
		__utl_scale_one_quickly(scale->param.inw, scale->param.inh, scale->param.outw, scale->param.outh, inbuf, outbuf, scale->tbuf);
		__utl_scale_one_quickly(
				scale->param.inw/2,
				scale->param.inh/2,
				scale->param.outw/2,
				scale->param.outh/2,
				inbuf+(scale->param.inw*scale->param.inh),
				outbuf+(scale->param.outw*scale->param.outh),
				scale->tbuf);
		__utl_scale_one_quickly(
				scale->param.inw/2,
				scale->param.inh/2,
				scale->param.outw/2,
				scale->param.outh/2,
				inbuf+(scale->param.inw*scale->param.inh*5/4),
				outbuf+(scale->param.outw*scale->param.outh*5/4),
				scale->tbuf);
	}
	else
	{

		__utl_scale_one(scale->param.inw, scale->param.inh, scale->param.outw, scale->param.outh, inbuf, outbuf, scale->tbuf);
		__utl_scale_one(
				scale->param.inw/2,
				scale->param.inh/2,
				scale->param.outw/2,
				scale->param.outh/2,
				inbuf+(scale->param.inw*scale->param.inh),
				outbuf+(scale->param.outw*scale->param.outh),
				scale->tbuf);
		__utl_scale_one(
				scale->param.inw/2,
				scale->param.inh/2,
				scale->param.outw/2,
				scale->param.outh/2,
				inbuf+(scale->param.inw*scale->param.inh*5/4),
				outbuf+(scale->param.outw*scale->param.outh*5/4),
				scale->tbuf);
	}
	return 0;
}

#else

/**
 *@brief 缩放
 *
 *@param handle 缩放器句柄
 *@param inbuf 输入参数。其长度由UtlScalParam_t中的参数计算得出
 *@param outbuf 输出参数。其长度由UtlScalParam_t中的参数计算得出
 */
int utl_scale(SCALE_HANDLE handle, const unsigned char *inbuf, unsigned char *outbuf)
{
	Scale_t *scale = (Scale_t *)handle;
	if (!handle)
	{
		printf("ERROR: scale destroy param error\n");
		return -1;
	}

	int pic_width_in  = scale->param.inw;
	int pic_height_in = scale->param.inh;
	int pic_width_out  = scale->param.outw;
	int pic_height_out = scale->param.outh;

	int width_scaler_cof = pic_width_in * 256 / pic_width_out;
	int height_scaler_cof = pic_height_in * 256 / pic_height_out;

	int i,j,k;

	int width_scaler_cnt = 0;//水平缩放系数累加器，低8bit表示相位，其余bit表示目标像素在源图像中最靠近它的左边一个像素
	int height_scaler_cnt = 0;//垂直缩放系数累加器，低8bit表示相位，其余bit表示目标像素在源图像中最靠近它的上边一个像素


	// ============================================================
	// Read input YUV file data


	// Read Y data
	unsigned char *p = (unsigned char *)inbuf;
	int offset;
	for (j=0,offset=0;j<scale->param.inh;j++,offset+=scale->pitch)
	{
		for (i=0;i<scale->param.inw;i++)
		{
			scale->yuva[0][offset+i] = *p++;
		}
	}

	// Read U data
	for( j=0; j<pic_height_in/2; j++ )
	{
		for( i=0; i<pic_width_in/2; i++ )
		{
			scale->yuva[1][(j*2+0)*scale->pitch+i*2+0] = *p;
			scale->yuva[1][(j*2+0)*scale->pitch+i*2+1] = *p;
			scale->yuva[1][(j*2+1)*scale->pitch+i*2+0] = *p;
			scale->yuva[1][(j*2+1)*scale->pitch+i*2+1] = *p;
			p++;
		}
	}

	// Read V data
	for( j=0; j<pic_height_in/2; j++ )
	{
		for( i=0; i<pic_width_in/2; i++ )
		{
			scale->yuva[2][(j*2+0)*scale->pitch+i*2+0] = *p;
			scale->yuva[2][(j*2+0)*scale->pitch+i*2+1] = *p;
			scale->yuva[2][(j*2+1)*scale->pitch+i*2+0] = *p;
			scale->yuva[2][(j*2+1)*scale->pitch+i*2+1] = *p;
			p++;
		}
	}

	// ============================================================
	// 水平方向缩放
	offset = 0;
	for( j=0; j<pic_height_in; j++ , offset += scale->pitch)
	{
		int left_pixel_data[3];
		int left_pixel_pos = 0;

		width_scaler_cnt = 0;

		// 预读取线性插值需要的两个像素中的左边一个像素
		left_pixel_data[0] = scale->yuva[0][j*scale->pitch];//rgb_in_data[0][j][0];
		left_pixel_data[1] = scale->yuva[1][j*scale->pitch];//rgb_in_data[0][j][1];
		left_pixel_data[2] = scale->yuva[2][j*scale->pitch];//rgb_in_data[0][j][2];

		for( i=0; i<pic_width_out; i++ )
		{
			int pixel_pos = width_scaler_cnt >> 8;//目标点左边像素
			int h_scaler_dist_r = 256 - (width_scaler_cnt & 0xff);//目标点离右边像素的距离
			int h_scaler_dist_l = width_scaler_cnt - left_pixel_pos;//目标点离被选取去插值的左边像素的距离

			int right_pixel_data[3];

			for( k=0; k<3; k++ )
			{
				int pixel_out_temp;
				int h_scaler_coef_r, h_scaler_coef_l;

				right_pixel_data[k] = scale->yuva[k][j*scale->pitch+pixel_pos+1];//rgb_in_data[pixel_pos+1][j][p];


#ifdef SCALER_JOHN_2ND

				if( (h_scaler_dist_r + h_scaler_dist_l) & (1<<11) )
				{
					h_scaler_coef_r = ((h_scaler_dist_l & 0xff) >> 3) | 0xe0;
					h_scaler_coef_l = 256 - h_scaler_coef_r;
				}
				else if( (h_scaler_dist_r + h_scaler_dist_l) & (1<<10) )
				{
					h_scaler_coef_r = ((h_scaler_dist_l & 0xff) >> 2) | 0xc0;
					h_scaler_coef_l = 256 - h_scaler_coef_r;
				}
				else if( (h_scaler_dist_r + h_scaler_dist_l) & (1<<9) )
				{
					h_scaler_coef_r = ((h_scaler_dist_l & 0xff) >> 1) | 0x80;
					h_scaler_coef_l = 256 - h_scaler_coef_r;
				}
				else if( (h_scaler_dist_r + h_scaler_dist_l) & (1<<8) )
				{
					h_scaler_coef_r = h_scaler_dist_l;
					h_scaler_coef_l = 256 - h_scaler_coef_r;
				}
				else
				{
					h_scaler_coef_r = 0;
					h_scaler_coef_l = 0;
				}
#else
				// 缩放的线性插值系数计算，待VS-LR确认并修改
				if( h_scaler_dist_r + h_scaler_dist_l == 256 )//插值点在左右两像素之间
				{
					h_scaler_coef_r = h_scaler_dist_l;//注意相位调换，离得越近，插值权重越大
					h_scaler_coef_l = h_scaler_dist_r;
				}
				else if( h_scaler_dist_r + h_scaler_dist_l == 512 )//插值点的左右两像素之间有一个被跳过的像素
				{
					h_scaler_coef_r = h_scaler_dist_l >> 1;
					h_scaler_coef_l = h_scaler_dist_r >> 1;
				}
				else
				{
					h_scaler_coef_r = h_scaler_dist_l / ((h_scaler_dist_r + h_scaler_dist_l) >> 8);
					h_scaler_coef_l = h_scaler_dist_r / ((h_scaler_dist_r + h_scaler_dist_l) >> 8);
				}
#endif

				pixel_out_temp = (h_scaler_coef_r * right_pixel_data[k]) + (h_scaler_coef_l * left_pixel_data[k]);
//				rgb_out_data[i][j][p] = (pixel_out_temp + 128) >> 8;//四舍五入
				scale->yuvb[k][j*scale->pitch+i] = (pixel_out_temp + 128) >> 8;//四舍五入
			}

			if( (width_scaler_cnt & ~0xff) != ((width_scaler_cnt + width_scaler_cof) & ~0xff) )
			{
				left_pixel_pos = (width_scaler_cnt & ~0xff) + 256;//当前插值的右边像素变成下次插值的左边像素
//				left_pixel_data[0] = rgb_in_data[pixel_pos+1][j][0];//拷贝右边像素到左边像素位置，为下次插值做准备
//				left_pixel_data[1] = rgb_in_data[pixel_pos+1][j][1];
//				left_pixel_data[2] = rgb_in_data[pixel_pos+1][j][2];
				left_pixel_data[0] = scale->yuva[0][j*scale->pitch+pixel_pos+1];//rgb_in_data[0][j][0];
				left_pixel_data[1] = scale->yuva[1][j*scale->pitch+pixel_pos+1];//rgb_in_data[0][j][1];
				left_pixel_data[2] = scale->yuva[2][j*scale->pitch+pixel_pos+1];//rgb_in_data[0][j][2];
			}
			width_scaler_cnt += width_scaler_cof;//插值系数累加到下一个点
		}
	}


	// ============================================================
	// 垂直方向缩放
	for( i=0; i<pic_width_out; i++ )
	{
		int left_pixel_data[3];
		int left_pixel_pos = 0;

		width_scaler_cnt = 0;

		// 预读取线性插值需要的两个像素中的左边一个像素
//		left_pixel_data[0] = rgb_out_data[i][0][0];
//		left_pixel_data[1] = rgb_out_data[i][0][1];
//		left_pixel_data[2] = rgb_out_data[i][0][2];
		left_pixel_data[0] = scale->yuvb[0][i];
		left_pixel_data[1] = scale->yuvb[1][i];
		left_pixel_data[2] = scale->yuvb[2][i];

		for( j=0; j<pic_height_out; j++ )
		{
			int pixel_pos = width_scaler_cnt >> 8;//目标点左边像素
			int h_scaler_dist_r = 256 - (width_scaler_cnt & 0xff);//目标点离右边像素的距离
			int h_scaler_dist_l = width_scaler_cnt - left_pixel_pos;//目标点离被选取去插值的左边像素的距离

			int right_pixel_data[3];

			for( k=0; k<3; k++ )
			{
				int pixel_out_temp;
				int h_scaler_coef_r, h_scaler_coef_l;

				right_pixel_data[k] = scale->yuvb[k][(pixel_pos+1)*scale->pitch+i];//rgb_out_data[i][pixel_pos+1][p];

#ifdef SCALER_JOHN_2ND

				if( (h_scaler_dist_r + h_scaler_dist_l) & (1<<11) )
				{
					h_scaler_coef_r = ((h_scaler_dist_l & 0xff) >> 3) | 0xe0;
					h_scaler_coef_l = 256 - h_scaler_coef_r;
				}
				else if( (h_scaler_dist_r + h_scaler_dist_l) & (1<<10) )
				{
					h_scaler_coef_r = ((h_scaler_dist_l & 0xff) >> 2) | 0xc0;
					h_scaler_coef_l = 256 - h_scaler_coef_r;
				}
				else if( (h_scaler_dist_r + h_scaler_dist_l) & (1<<9) )
				{
					h_scaler_coef_r = ((h_scaler_dist_l & 0xff) >> 1) | 0x80;
					h_scaler_coef_l = 256 - h_scaler_coef_r;
				}
				else if( (h_scaler_dist_r + h_scaler_dist_l) & (1<<8) )
				{
					h_scaler_coef_r = h_scaler_dist_l;
					h_scaler_coef_l = 256 - h_scaler_coef_r;
				}
				else
				{
					h_scaler_coef_r = 0;
					h_scaler_coef_l = 0;
				}
#else
				// 缩放的线性插值系数计算，待VS-LR确认并修改
				if( h_scaler_dist_r + h_scaler_dist_l == 256 )//插值点在左右两像素之间
				{
					h_scaler_coef_r = h_scaler_dist_l;//注意相位调换，离得越近，插值权重越大
					h_scaler_coef_l = h_scaler_dist_r;
				}
				else if( h_scaler_dist_r + h_scaler_dist_l == 512 )//插值点的左右两像素之间有一个被跳过的像素
				{
					h_scaler_coef_r = h_scaler_dist_l >> 1;
					h_scaler_coef_l = h_scaler_dist_r >> 1;
				}
				else
				{
					h_scaler_coef_r = h_scaler_dist_l / ((h_scaler_dist_r + h_scaler_dist_l) >> 8);
					h_scaler_coef_l = h_scaler_dist_r / ((h_scaler_dist_r + h_scaler_dist_l) >> 8);
				}
#endif

				pixel_out_temp = (h_scaler_coef_r * right_pixel_data[k]) + (h_scaler_coef_l * left_pixel_data[k]);
//				rgb_in_data[i][j][p] = (pixel_out_temp + 128) >> 8;//四舍五入
				scale->yuva[k][j*scale->pitch+i] = (pixel_out_temp + 128) >> 8;//四舍五入
			}

			if( (width_scaler_cnt & ~0xff) != ((width_scaler_cnt + height_scaler_cof) & ~0xff) )
			{
				left_pixel_pos = (width_scaler_cnt & ~0xff) + 256;//当前插值的右边像素变成下次插值的左边像素
				left_pixel_data[0] = scale->yuvb[0][(pixel_pos+1)*scale->pitch+i];//rgb_out_data[i][pixel_pos+1][0];//拷贝右边像素到左边像素位置，为下次插值做准备
				left_pixel_data[1] = scale->yuvb[1][(pixel_pos+1)*scale->pitch+i];//rgb_out_data[i][pixel_pos+1][1];
				left_pixel_data[2] = scale->yuvb[2][(pixel_pos+1)*scale->pitch+i];//rgb_out_data[i][pixel_pos+1][2];
			}
			width_scaler_cnt += height_scaler_cof;//插值系数累加到下一个点
		}
	}

	p = outbuf;

	for (j=0,offset=0;j<scale->param.outh;j++,offset+=scale->pitch)
	{
		for (i=0;i<scale->param.outw;i++)
		{
			*p++ = scale->yuva[0][offset+i];
		}
	}

	// Read U data
	for( j=0; j<pic_height_out/2; j++ )
	{
		for( i=0; i<pic_width_out/2; i++ )
		{
			*p++ = scale->yuva[1][(j*2+0)*scale->pitch+i*2+0];
		}
	}

	// Read V data
	for( j=0; j<pic_height_out/2; j++ )
	{
		for( i=0; i<pic_width_out/2; i++ )
		{
			*p++ = scale->yuva[2][(j*2+0)*scale->pitch+i*2+0];
		}
	}


//	// ============================================================
//	// 转换YUV图像为RGB图像
//	for( j=0; j<pic_width_out; j++ )
//	{
//		for( i=0; i<pic_height_out; i++ )
//		{
//			float r,g,b;
//			int y = rgb_in_data[j][i][0];
//			int u = rgb_in_data[j][i][1];
//			int v = rgb_in_data[j][i][2];
//
//			y = (y<16)? 16-16  : ((y>235)? 235-16  : y-16 );
//			u = (u<16)? 16-128 : ((u>240)? 240-128 : u-128 );
//			v = (v<16)? 16-128 : ((v>240)? 240-128 : v-128 );
//
//			r =  1.164 * y + 1.596*v;
//			g =  1.164 * y - 0.813*v - 0.391*u;
//			b =  1.164 * y + 2.018*u;
//
//			r = (r<0)? 0 : ((r>255)? 255 : r);
//			g = (g<0)? 0 : ((g>255)? 255 : g);
//			b = (b<0)? 0 : ((b>255)? 255 : b);
//
//			rgb_out_data[j][i][0] = (unsigned char)r;
//			rgb_out_data[j][i][1] = (unsigned char)g;
//			rgb_out_data[j][i][2] = (unsigned char)b;
//		}
//	}
	return 0;
}

#endif

/**
 *@brief 销毁缩放器
 */
int utl_scale_destroy(SCALE_HANDLE handle)
{
	Scale_t *scale = (Scale_t *)handle;
	if (!handle)
	{
		printf("ERROR: scale destroy param error\n");
		return -1;
	}
#if SIMPLE_METHOD
//	if (scale->tbuf)
//		free(scale->tbuf);
#else
	if (scale->ptr)
		free(scale->ptr);
#endif
	free (scale);
	return 0;
}

#if 0

int main(int argc, char *argv[])
{
#define PIC_WIDTH	704
#define PIC_HEIGHT	576
	int width_scaler_cof  = 512;//水平方向上的缩放系数，设置值 = 源图像水平尺寸 / 目标图像水平尺寸 * 256
	int height_scaler_cof = 512;//垂直方向上的缩放系数，设置值 = 源图像垂直尺寸 / 目标图像垂直尺寸 * 256


	unsigned char bmp_head[54];//存储BMP头部数据
	FILE *fp;

	int pic_width_in  = PIC_WIDTH;
	int pic_height_in = PIC_HEIGHT;

	int pic_width_out  = pic_width_in * 256 / width_scaler_cof;
	int pic_height_out = pic_height_in * 256 / height_scaler_cof;
	int pic_size_out;
	int pic_width_out_temp;

	int i,j,p;

	int width_scaler_cnt = 0;//水平缩放系数累加器，低8bit表示相位，其余bit表示目标像素在源图像中最靠近它的左边一个像素
	int height_scaler_cnt = 0;//垂直缩放系数累加器，低8bit表示相位，其余bit表示目标像素在源图像中最靠近它的上边一个像素

printf("in: %dx%d, out: %dx%d\n", pic_width_in, pic_height_in, pic_width_out, pic_height_out);
pic_width_out +=3;
pic_height_out += 3;
pic_width_out &= ~3;
pic_height_out &= ~3;

	static unsigned char in[1920*1080*3/2];
	static unsigned char out[1920*1080*3/2];
	int ret;

	fp  = fopen( "in.bmp", "rb" );
	for( i=0; i<54; i++ )
		bmp_head[i] = fgetc(fp);
	fclose(fp);

	fp  = fopen( "in.yuv", "rb" );
	ret = fread(in, 1, 1920*1080*3/2, fp);
	printf("readed: %d\n", ret);
	fclose(fp);
	UtlScalParam_t param;
	param.dataFmt = UTL_SCALE_FMT_YUV420;
	param.bQuickly = 1;
	param.inw = pic_width_in;
	param.inh = pic_height_in;
	param.outw = pic_width_out;
	param.outh = pic_height_out;
	SCALE_HANDLE handle = utl_scale_create(&param);
	if (!handle)
	{
		printf("ERROR: Failed create scale handle\n");
		return -1;
	}

	for (i=0;i<100;i++)
	ret = utl_scale(handle, in, out);
	if (ret < 0)
	{
		printf("Failed scale\n");
		return -1;
	}

	// write bmp
	fp = fopen( "out.bmp", "wb" );
	pic_width_out_temp  = (pic_width_out  + 4) & ~0x3;

	pic_size_out = pic_width_out * pic_height_out * 3;

	// 写BMP头
	bmp_head[2] = (pic_size_out + 54) & 0xff;
	bmp_head[3] = ((pic_size_out + 54) >> 8)  & 0xff;
	bmp_head[4] = ((pic_size_out + 54) >> 16) & 0xff;

	bmp_head[18] = pic_width_out_temp & 0xff;
	bmp_head[19] = (pic_width_out_temp >> 8) & 0xff;

	bmp_head[22] = pic_height_out & 0xff;
	bmp_head[23] = (pic_height_out >> 8) & 0xff;

	bmp_head[34] = pic_size_out & 0xff;
	bmp_head[35] = (pic_size_out >> 8)  & 0xff;
	bmp_head[36] = (pic_size_out >> 16) & 0xff;

	for( i=0; i<54; i++ ){
		fputc(bmp_head[i], fp);
	}


	unsigned char *py = out;
	unsigned char *pu = out + pic_width_out*pic_height_out;
	unsigned char *pv = pu + pic_width_out*pic_height_out/4;
	unsigned char rgb_out_data[1920][1080][3];
	for( j=0; j<pic_height_out; j++ )
	{
		for( i=0; i<pic_width_out; i++ )
		{
			float r,g,b;
			int y = py[j*pic_width_out+i];
			int u = pu[j/2*pic_width_out/2+i/2];//rgb_in_data[j][i][1];
			int v = pv[j/2*pic_width_out/2+i/2];;

			y = (y<16)? 16-16  : ((y>235)? 235-16  : y-16 );
			u = (u<16)? 16-128 : ((u>240)? 240-128 : u-128 );
			v = (v<16)? 16-128 : ((v>240)? 240-128 : v-128 );

			r =  1.164 * y + 1.596*v;
			g =  1.164 * y - 0.813*v - 0.391*u;
			b =  1.164 * y + 2.018*u;

			r = (r<0)? 0 : ((r>255)? 255 : r);
			g = (g<0)? 0 : ((g>255)? 255 : g);
			b = (b<0)? 0 : ((b>255)? 255 : b);

//			fputc(r, fp);
//			fputc(g, fp);
//			fputc(b, fp);
			rgb_out_data[i][j][0] = (unsigned char)r;
			rgb_out_data[i][j][1] = (unsigned char)g;
			rgb_out_data[i][j][2] = (unsigned char)b;
		}
	}


	//RGB data writer to bmp file
	for( j=0; j<pic_height_out; j++ )
	{
		for( i=0; i<pic_width_out_temp; i++ )
		{
			fputc( rgb_out_data[i][pic_height_out - j - 1][2] , fp);
			fputc( rgb_out_data[i][pic_height_out - j - 1][1] , fp);
			fputc( rgb_out_data[i][pic_height_out - j - 1][0] , fp);
		}
		for( ; i<pic_width_out; i++ )
		{
			fputc( 0 , fp);
			fputc( 0 , fp);
			fputc( 0 , fp);
		}
	}


	fclose(fp);
	return 0;
}

#endif
