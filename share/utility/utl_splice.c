#include "utl_timer.h"
#include "jpeglib.h"
#include "utl_splice.h"


/************************************************************
写bmp文件的头信息，jpeg格式转换为bmp时调用
************************************************************/
int write_bmp_header(j_decompress_ptr cinfo,FILE *output_file)
{
    struct bmp_fileheader bfh;
    struct bmp_infoheader bih;

    unsigned long width;
    unsigned long height;
    unsigned short depth;
    unsigned long headersize;
    unsigned long filesize=0;

	int i=0;
	unsigned char j=0;
	unsigned char *platte;

    width=cinfo->output_width;
    height=cinfo->output_height;
    depth=cinfo->output_components;

    if (depth==1)        //灰度图像
    {
        headersize=14+40+256*4;
        filesize=headersize+width*height;
    }

    if (depth==3)        //彩色图像
    {
        headersize=14+40;
        filesize=headersize+width*height*depth;
    }

    memset(&bfh,0,sizeof(struct bmp_fileheader));
    memset(&bih,0,sizeof(struct bmp_infoheader));
    
    //写入比较关键的几个bmp头参数
    bfh.bfType=0x4D42;          //文件头，固定"BM"
    bfh.bfSize=filesize;        //文件大小，文件头大小+图像内容大小
    bfh.bfOffBits=headersize;

    bih.biSize=40;
    bih.biWidth=width;
    bih.biHeight=height;
    bih.biPlanes=1;
    bih.biBitCount=(unsigned short)depth*8;
    bih.biSizeImage=width*height*depth;

    fwrite(&bfh,sizeof(struct bmp_fileheader),1,output_file);
    fwrite(&bih,sizeof(struct bmp_infoheader),1,output_file);

    if (depth==1)        //灰度图像要添加调色板
    {
        platte = (unsigned char *)malloc(sizeof(unsigned char)*256*4);
        if (NULL == platte)
        {
			printf("write_bmp_header : malloc platte mem failed !\n");
			return VOS_ERR;
        }
        
        for(i=0;i<1024;i+=4)
        {
            platte[i]=j;
            platte[i+1]=j;
            platte[i+2]=j;
            platte[i+3]=0;
            j++;
        }
        fwrite(platte,sizeof(unsigned char)*1024,1,output_file);
        free(platte);
		platte = NULL;
    }

	return VOS_OK;
}

/************************************************************
写bmp文件的图像内容，jpeg格式转换为bmp时调用
************************************************************/
int write_bmp_data(j_decompress_ptr cinfo,unsigned char *src_buff,FILE *output_file)
{
    unsigned char *dst_width_buff;
    unsigned char *point;

    unsigned long width;
    unsigned long height;
    unsigned short depth;

	unsigned long i=0;
	unsigned long j=0;

    width=cinfo->output_width;
    height=cinfo->output_height;
    depth=cinfo->output_components;

    dst_width_buff = (unsigned char *)malloc(sizeof(unsigned char)*width*depth);
	if (NULL == dst_width_buff)
	{
		printf("write_bmp_data : malloc dst_width_buff mem failed !\n");
		return VOS_ERR;
	}
    memset(dst_width_buff,0,sizeof(unsigned char)*width*depth);

    point=src_buff+width*depth*(height-1);    //倒着写数据，bmp格式是倒的，jpg是正的
    for (i=0;i<height;i++)
    {
        for (j=0;j<width*depth;j+=depth)
        {
            if (depth==1)        //处理灰度图
            {
                dst_width_buff[j]=point[j];
            }

            if (depth==3)        //处理彩色图
            {
                dst_width_buff[j+2]=point[j+0];
                dst_width_buff[j+1]=point[j+1];
                dst_width_buff[j+0]=point[j+2];
            }
        }
        point-=width*depth;
        fwrite(dst_width_buff,sizeof(unsigned char)*width*depth,1,output_file);    //一次写一行
    }

	free(dst_width_buff);
	dst_width_buff = NULL;

	return VOS_OK;
}

/************************************************************
对JPEG文件进行格式转换，转换成BMP格式，并存放到临时文件中
************************************************************/
int Jpeg2BMP(char *FileName,char *tmpBMPFileName)
{
	
	FILE *fp , *fp_tmp; 
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPARRAY buffer;
    unsigned char *src_buff;
    unsigned char *point;

	unsigned long Width;
	unsigned long Height;
	unsigned short depth;

	//合法性校验，指针不能为空
	if (NULL == FileName || NULL == tmpBMPFileName)
	{
		printf("Jpeg2BMP : input param error !\n");
		return VOS_ERR;
	}

	//打开源JPEG文件
	fp = fopen(FileName,"rb");
	if(NULL == fp)
	{
		printf("Jpeg2BMP : open source file %s error !\n", FileName);
		return VOS_ERR;
	}

	//打开目的BMP文件
	fp_tmp = fopen(tmpBMPFileName,"wb");
	if(NULL == fp_tmp)
	{
		printf("Jpeg2BMP : open dest file %s error !\n", tmpBMPFileName);
		fclose(fp);                //如果出现异常，之前malloc和fopen相关的都要处理，防止内存泄露
		return VOS_ERR;
	}

	cinfo.err=jpeg_std_error(&jerr);    ////以下为libjpeg.a库提供的API接口，具体参看相关文档

	jpeg_create_decompress(&cinfo);  

	jpeg_stdio_src(&cinfo, fp);

	jpeg_read_header(&cinfo, TRUE);

	//开始解码JPEG文件
	jpeg_start_decompress(&cinfo);

	Width = cinfo.output_width;
    Height = cinfo.output_height;
	depth = cinfo.output_components;

	src_buff = (unsigned char *)malloc(sizeof(unsigned char)*Width*Height*depth);
	if (NULL == src_buff)
	{
		printf("Jpeg2BMP : malloc src_buff mem failed !\n");
		fclose(fp);
		fclose(fp_tmp);
		return VOS_ERR;
	}
    memset(src_buff,0,sizeof(unsigned char)*Width*Height*depth);

    buffer=(*cinfo.mem->alloc_sarray)
        ((j_common_ptr)&cinfo,JPOOL_IMAGE,Width*depth,1);

    point=src_buff;
    while (cinfo.output_scanline<Height)
    {
        jpeg_read_scanlines(&cinfo,buffer,1);    //读取一行jpg图像数据到buffer
        memcpy(point,*buffer,Width*depth);    //将buffer中的数据逐行给src_buff
        point+=Width*depth;            //一次改变一行
    }

	if(VOS_OK != write_bmp_header(&cinfo,fp_tmp))            //写bmp文件头
	{
		printf("Jpeg2BMP : write_bmp_header return error !\n");
		fclose(fp);
		fclose(fp_tmp);
		free(src_buff);
		src_buff = NULL;
		return VOS_ERR;
	}

    if(VOS_OK != write_bmp_data(&cinfo,src_buff,fp_tmp))    //写bmp像素数据
	{
		printf("Jpeg2BMP : write_bmp_data return error !\n");
		fclose(fp);
		fclose(fp_tmp);
		free(src_buff);
		src_buff = NULL;
		return VOS_ERR;
	}

	//资源释放
	jpeg_finish_decompress(&cinfo);

	jpeg_destroy_decompress(&cinfo);

	fclose(fp);
	fclose(fp_tmp);
	free(src_buff);
	src_buff = NULL;

	return VOS_OK;
}

/************************************************************
把两个bmp文件上下拼接成一个bmp文件，要求两个文件的width和depth相同
************************************************************/
int BMPSplice(char *FileName,char *FileName2,char *destFileName)
{
	FILE *fp,*fp2,*fp_dest; 

	struct bmp_fileheader bfh,bfh2,bfh_dest;    //分别代表两个源文件和一个目的文件的文件头
    struct bmp_infoheader bih,bih2,bih_dest;

	unsigned long width;
    unsigned long height;
    unsigned short depth;

	unsigned char *dst_width_buff;
	unsigned char *src_buff;
    unsigned char *point;
	unsigned long i=0;
	unsigned long j=0;

	//入参合法性校验，不得为空
	if (NULL == FileName || NULL == FileName2 || NULL == destFileName)
	{
		printf("BMPSplice : input param error !\n");
		return VOS_ERR;
	}

	fp = fopen(FileName,"rb");
	if(NULL == fp)
	{
		printf("BMPSplice : open 1st src file error !\n");
		return VOS_ERR;
	}

	fp2 = fopen(FileName2,"rb");
	if(NULL == fp2)
	{
		printf("BMPSplice : open 2nd src file error !\n");
		fclose(fp);
		return VOS_ERR;
	}

	fp_dest = fopen(destFileName,"wb");
	if(NULL == fp_dest)
	{
		printf("BMPSplice : open dest file error !\n");
		fclose(fp);
		fclose(fp2);
		return VOS_ERR;
	}

	memset(&bfh,0,sizeof(struct bmp_fileheader));
    memset(&bih,0,sizeof(struct bmp_infoheader));
	memset(&bfh2,0,sizeof(struct bmp_fileheader));
    memset(&bih2,0,sizeof(struct bmp_infoheader));
	memset(&bfh_dest,0,sizeof(struct bmp_fileheader));
    memset(&bih_dest,0,sizeof(struct bmp_infoheader));

	fread(&bfh,sizeof(struct bmp_fileheader),1,fp);
    fread(&bih,sizeof(struct bmp_infoheader),1,fp);
	fread(&bfh2,sizeof(struct bmp_fileheader),1,fp2);
    fread(&bih2,sizeof(struct bmp_infoheader),1,fp2);

	//如果两个图的宽度不同，或者图像类型不同，则不处理
	if (bih.biWidth != bih2.biWidth || bih.biBitCount != bih2.biBitCount)
	{
		printf("BMPSplice : two src files have different width and depth !\n");
		fclose(fp);
		fclose(fp2);
		fclose(fp_dest);
		return VOS_ERR;
	}
	
	width = bih.biWidth;
	height = (bih.biHeight + bih2.biHeight);
	depth = (bih.biBitCount/8);

	//写入比较关键的几个bmp头参数
    bfh_dest.bfType=0x4D42;
    bfh_dest.bfSize=(14+40) + width*height*depth;
    bfh_dest.bfOffBits=14+40;

    bih_dest.biSize=40;
    bih_dest.biWidth=width;
    bih_dest.biHeight=height;
    bih_dest.biPlanes=1;
    bih_dest.biBitCount=bih.biBitCount;
    bih_dest.biSizeImage=width*height*depth;

	//写入修改后的目的文件的文件头
	fwrite(&bfh_dest,sizeof(struct bmp_fileheader),1,fp_dest);
    fwrite(&bih_dest,sizeof(struct bmp_infoheader),1,fp_dest);

	dst_width_buff = (unsigned char *)malloc(sizeof(unsigned char)*width*depth);
	if (NULL == dst_width_buff)
	{
		printf("BMPSplice : malloc dst_width_buff mem failed !\n");
		fclose(fp);
		fclose(fp2);
		fclose(fp_dest);
		return VOS_ERR;
	}
    memset(dst_width_buff,0,sizeof(unsigned char)*width*depth);

	src_buff = (unsigned char *)malloc(sizeof(unsigned char)*width*(bih.biHeight)*depth);
	if (NULL == src_buff)
	{
		printf("BMPSplice : malloc src_buff mem failed !\n");
		fclose(fp);
		fclose(fp2);
		fclose(fp_dest);
		free(dst_width_buff);
		dst_width_buff = NULL;
		return VOS_ERR;
	}
    memset(src_buff,0,sizeof(unsigned char)*width*(bih.biHeight)*depth);

	fread(src_buff,1,width*(bih.biHeight)*depth,fp);
	point=src_buff;
	for (i = 0 ; i < bih.biHeight ; i++)
	{
		for (j=0;j<width*depth;j+=depth)
        {
            dst_width_buff[j+0]=point[j+0];
            dst_width_buff[j+1]=point[j+1];
            dst_width_buff[j+2]=point[j+2];
        }
        point+=width*depth;
		fwrite(dst_width_buff,sizeof(unsigned char)*width*depth,1,fp_dest);    //一次写一行
	}

	free(src_buff);
	src_buff = NULL;

	//为了防止BMP1和BMP2的高不同导致图片显示异常，所以释放以后重新申请
	src_buff = (unsigned char *)malloc(sizeof(unsigned char)*width*(bih2.biHeight)*depth);
	if (NULL == src_buff)
	{
		printf("BMPSplice : malloc src_buff mem again failed !\n");
		fclose(fp);
		fclose(fp2);
		fclose(fp_dest);
		free(dst_width_buff);
		dst_width_buff = NULL;
		return VOS_ERR;
	}
    memset(src_buff,0,sizeof(unsigned char)*width*(bih.biHeight)*depth);

	fread(src_buff,1,width*(bih2.biHeight)*depth,fp2);
	point=src_buff;
	for (i = bih.biHeight ; i < height ; i++)
	{
		for (j=0;j<width*depth;j+=depth)
        {
            dst_width_buff[j+0]=point[j+0];
            dst_width_buff[j+1]=point[j+1];
            dst_width_buff[j+2]=point[j+2];
        }
        point+=width*depth;
		fwrite(dst_width_buff,sizeof(unsigned char)*width*depth,1,fp_dest);    //一次写一行
	}

	//资源释放
	free(dst_width_buff);
	free(src_buff);
	dst_width_buff = NULL;
	src_buff = NULL;
	fclose(fp);
	fclose(fp2);
	fclose(fp_dest);

	return VOS_OK;
}

/************************************************************
读取bmp文件的图像内容并调整顺序，bmp文件格式转换为jpeg时调用
************************************************************/
int read_bmp_data(unsigned char *src_buff,unsigned long width,unsigned long height,unsigned short depth)  
{  
    unsigned char *src_point;  
    unsigned char *dst_point;  
	unsigned char *tmp_buff;
	unsigned long i,j;

	//入参合法性校验
	if (NULL == src_buff)
	{
		printf("read_bmp_data : input param error ! \n");
		return VOS_ERR;
	}

	tmp_buff = (unsigned char *)malloc(sizeof(unsigned char)*width*height*depth);
	if (NULL == tmp_buff)
	{
		printf("read_bmp_data : malloc tmp_buff mem failed ! \n");
		return VOS_ERR;
	}
	memset(tmp_buff,0,(sizeof(unsigned char)*width*height*depth));
  
    src_point=src_buff+width*depth*(height-1);  
    dst_point=tmp_buff+width*depth*(height-1);  
    for (i=0;i<height;i++)  
    {  
        for (j=0;j<width*depth;j+=depth)  
        {  
            if (depth==1)        //处理灰度图  
            {  
                dst_point[j]=src_point[j];  
            }  
  
            if (depth==3)        //处理彩色图  
            {  
                dst_point[j+2]=src_point[j+0];     //调整顺序，BMP和JPEG的数据顺序是相反的
                dst_point[j+1]=src_point[j+1];  
                dst_point[j+0]=src_point[j+2];  
            }  
        }  
        dst_point-=width*depth;  
        src_point-=width*depth;  
    }  

	//将调整后的数据重新复制到源指针中，并释放资源
	memcpy(src_buff,tmp_buff,(sizeof(unsigned char)*width*height*depth));
	free(tmp_buff);
	tmp_buff = NULL;

	return VOS_OK;
}

/************************************************************
将bmp的源文件格式转换为jpeg的目标文件
************************************************************/
int BMP2Jpeg(char *FileName,char *dest_JpegFileName)
{
	FILE *fp_bmp,*fp_jpeg;

	struct bmp_fileheader bfh;
    struct bmp_infoheader bih;

	unsigned long width;
    unsigned long height;
    unsigned short depth;

	unsigned char *src_buff;

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPARRAY buffer;
	unsigned char *point;

	//入参合法性校验
	if (NULL == FileName || NULL == dest_JpegFileName)
	{
		printf("BMP2Jpeg : input param error !\n");
		return VOS_ERR;
	}

	fp_bmp = fopen(FileName,"rb");
	if(NULL == fp_bmp)
	{
		printf("BMP2Jpeg : open bmp file error !\n");
		return VOS_ERR;
	}

	fp_jpeg = fopen(dest_JpegFileName,"wb");
	if(NULL == fp_jpeg)
	{
		printf("BMP2Jpeg : open jpeg file error !\n");
		fclose(fp_bmp);
		return VOS_ERR;
	}

	memset(&bfh,0,sizeof(struct bmp_fileheader));
    memset(&bih,0,sizeof(struct bmp_infoheader));
	
	fread(&bfh,sizeof(struct bmp_fileheader),1,fp_bmp);
    fread(&bih,sizeof(struct bmp_infoheader),1,fp_bmp);

	width = bih.biWidth;
	height = bih.biHeight;
	depth = (bih.biBitCount/8);

	src_buff = (unsigned char *)malloc(sizeof(unsigned char)*width*height*depth);
	if (NULL == src_buff)
	{
		printf("BMP2Jpeg : malloc src_buff mem failed !\n");
		fclose(fp_bmp);
		fclose(fp_jpeg);
		return VOS_ERR;
	}
    memset(src_buff,0,sizeof(unsigned char)*width*height*depth);

	fread(src_buff,width*height*depth,1,fp_bmp);

	if(VOS_OK != read_bmp_data(src_buff,width,height,depth))   //读取BMP文件信息，并进行调整
	{
		printf("BMP2Jpeg : read_bmp_data return error !\n");
		fclose(fp_bmp);
		fclose(fp_jpeg);
		free(src_buff);
		src_buff = NULL;
		return VOS_ERR;
	}

	cinfo.err = jpeg_std_error(&jerr);         //以下为libjpeg.a库文件的API接口，具体可参考相关文档

	jpeg_create_compress(&cinfo); 
	
	jpeg_stdio_dest(&cinfo, fp_jpeg);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = depth; 
	if (depth==1)
	{
        cinfo.in_color_space = JCS_GRAYSCALE;
	}
    else
	{
        cinfo.in_color_space = JCS_RGB;
	}

	jpeg_set_defaults(&cinfo);

	jpeg_set_quality(&cinfo, JPEG_QUALITY, TRUE );

	jpeg_start_compress(&cinfo, TRUE);

	buffer=(*cinfo.mem->alloc_sarray)
            ((j_common_ptr)&cinfo,JPOOL_IMAGE,width*depth,1);
	point=src_buff+width*depth*(height-1);
    while (cinfo.next_scanline < height)
    {
        memcpy(*buffer,point,width*depth);          //将处理OK后的数据复制到jpeg文件中，一次一行
        jpeg_write_scanlines(&cinfo,buffer,1);
        point-=width*depth;
    }

	//资源释放
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	free(src_buff);
	src_buff = NULL;
	fclose(fp_bmp);
	fclose(fp_jpeg);

	return VOS_OK;
}

int GetBmpTmpFloder(char *srcPath, char *dstPath)
{
	if (!srcPath || !dstPath)
		return 0;

	char *p = strrchr(srcPath, '/');
	if (NULL == p)
		return 0;
	
	strncpy(dstPath, srcPath, p-srcPath);	
	return 0;
}

/************************************************************
将前面几个函数进行封装，只提供这一个接口，入参为两个源jpeg文件路径，和一个目的jpeg文件路径
************************************************************/
int SpliceInterface(char *FileName,char *FileName2,char *DestFileName)
{
	//入参判空
	if (NULL == FileName || NULL == FileName2 || NULL == DestFileName)
	{
		printf("SpliceInterface : input param error !\n");
		return VOS_ERR;
	}

	char dstPath[128];
	char tmp1Bmp[128];
	char tmp2Bmp[128];
	char tmp3Bmp[128];

	memset(dstPath, 0, 128);
	memset(tmp1Bmp, 0, 128);
	memset(tmp2Bmp, 0, 128);
	memset(tmp3Bmp, 0, 128);
	GetBmpTmpFloder(FileName, dstPath);

	sprintf(tmp1Bmp, "%s/%s", dstPath, "tmp1.bmp");
	sprintf(tmp2Bmp, "%s/%s", dstPath, "tmp2.bmp");
	sprintf(tmp3Bmp, "%s/%s", dstPath, "tmp3.bmp");
	
	/************************************************************
	1、两张图片分别转换为bmp格式
	************************************************************/
	if(VOS_OK != Jpeg2BMP(FileName, tmp1Bmp))
	{
		printf("SpliceInterface : 1st Jpeg2BMP return error !\n");
		return VOS_ERR;
	}
	
	if(VOS_OK != Jpeg2BMP(FileName2, tmp2Bmp))
	{
		printf("SpliceInterface : 2nd Jpeg2BMP return error !\n");
		return VOS_ERR;
	}	

	/************************************************************
	2、两张bmp文件进行上下拼接，合成一张图片
	************************************************************/
	if(VOS_OK != BMPSplice(tmp1Bmp, tmp2Bmp, tmp3Bmp))
	{
		printf("SpliceInterface : BMPSplice return error !\n");
		return VOS_ERR;
	}
	
	/************************************************************
	3、bmp文件格式转换为jpeg格式
	************************************************************/
	if(VOS_OK != BMP2Jpeg(tmp3Bmp, DestFileName))
	{
		printf("SpliceInterface : BMP2Jpeg return error !\n");
		return VOS_ERR;
	}	

	/************************************************************
	4、删除临时BMP文件
	************************************************************/
	(void)remove(tmp1Bmp);
	(void)remove(tmp2Bmp);
	(void)remove(tmp3Bmp);

	return VOS_OK;
}


