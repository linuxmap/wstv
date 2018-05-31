/******************************************************************************

  Copyright (c) 2012-2015 Jovision Technology Co., Ltd. All rights reserved.

  File Name     : splice.h
  Version       : 
  Author        : zhouwenqiang<zhouwenqiang@jovision.com>
  Created       : 2015-6-29
  Description   : Demo of jpeg splice
  History       : 
  Date        : 2015-6-29
  Modification: Created file
******************************************************************************/
#ifndef _M_SPLICE_H_
#define _M_SPLICE_H_

#include "jpeglib.h"

#pragma pack(2)        //两字节对齐，否则bmp_fileheader会占16Byte

#define JPEG_QUALITY 95
#define VOS_OK 		0 	// 0
#define VOS_ERR 	-1		// -1

struct bmp_fileheader
{
    unsigned short    bfType;        //若不对齐，这个会占4Byte
    unsigned long    bfSize;
    unsigned short    bfReverved1;
    unsigned short    bfReverved2;
    unsigned long    bfOffBits;
};

struct bmp_infoheader
{
    unsigned long    biSize;
    unsigned long    biWidth;
    unsigned long    biHeight;
    unsigned short    biPlanes;
    unsigned short    biBitCount;
    unsigned long    biCompression;
    unsigned long    biSizeImage;
    unsigned long    biXPelsPerMeter;
    unsigned long    biYpelsPerMeter;
    unsigned long    biClrUsed;
    unsigned long    biClrImportant;
};

int write_bmp_header(j_decompress_ptr cinfo, FILE *output_file);
int write_bmp_data(j_decompress_ptr cinfo,unsigned char *src_buff,FILE *output_file);
int Jpeg2BMP(char *FileName,char *tmpBMPFileName);
int BMPSplice(char *FileName,char *FileName2,char *destFileName);
int BMP2Jpeg(char *FileName,char *tmpBMPFileName);
int read_bmp_data(unsigned char *src_buff,unsigned long width,unsigned long height,unsigned short depth);
int SpliceInterface(char *FileName,char *FileName2,char *DestFileName);


#endif

