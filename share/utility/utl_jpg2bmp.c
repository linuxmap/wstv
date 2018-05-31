/*
 * utl_jpg2bmp.c
 *
 *  Created on: 2015-09-06
 *      Author: zwq
 *	  function: 不调用库的情况下，实现jpg格式转换为bmp的功能
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utl_jpg2bmp.h"

#define U8 		unsigned char
#define U16 	unsigned short int
#define U32 	unsigned int
#define S32 	signed int
#define S8 		signed char
#define S16 	signed short int

char error_string[90];
#define exit_func(err) { printf("%s\n",err); return 0;}


typedef struct s_BM_header {
	U16 BMP_id ; 		// 'B''M'
	U32 size; 			// size in bytes of the BMP file
	U32 zero_res; 		// 0
	U32 offbits; 		// 54
	U32 biSize; 		// 0x28
	U32 Width;  		// X
	U32 Height;  		// Y
	U16  biPlanes; 		// 1
	U16  biBitCount ; 	// 24
	U32 biCompression; 	// 0 = BI_RGB
	U32 biSizeImage; 	// 0
	U32 biXPelsPerMeter;// 0xB40
	U32 biYPelsPerMeter;// 0xB40
	U32 biClrUsed; 		//0
	U32 biClrImportant; //0
} BM_header;

typedef struct s_RGB {
	U8 B;
	U8 G;
	U8 R;
} RGB;

#define COLOR_FIX(r)   (((r) > (0xff-4)) ? (r) : (r)+3)
#define MY_RGB(r,g,b,a) (((COLOR_FIX(r)&0xf8)<<7)+((COLOR_FIX(g)&0xf8)<<2)+((COLOR_FIX(b)&0xf8)>>3))

//16 bit bmp
int write_buf_to_BMP(U8 *im_buffer, U16 X_bitmap, U16 Y_bitmap, U8 *pBmpData, int *bmpLen)
{
	S16 x, y ,t;
	RGB *pixel;
	BM_header BH;
	U32 im_loc_bytes;
	U8 nr_fillingbytes, i;
	U16 zero_byte=0;
	U8 *tmp = pBmpData;
	int tmpLen = 0;
	
	if (X_bitmap%2 != 0) 
		nr_fillingbytes = 1;
	else 
		nr_fillingbytes = 0;

	BH.BMP_id = 'M'*256+'B';			memcpy(tmp,&BH.BMP_id,2);			tmpLen += 2;			tmp = tmp + 2;
	BH.size=54+Y_bitmap*(X_bitmap*2);	memcpy(tmp,&BH.size,4);				tmpLen += 4;			tmp = tmp + 4;
	BH.zero_res = 0;					memcpy(tmp,&BH.zero_res,4);			tmpLen += 4;			tmp = tmp + 4;
	BH.offbits = 54;					memcpy(tmp,&BH.offbits,4);			tmpLen += 4;			tmp = tmp + 4;
	BH.biSize = 0x28;				memcpy(tmp,&BH.biSize,4);			tmpLen += 4;			tmp = tmp + 4;
	BH.Width = X_bitmap;				memcpy(tmp,&BH.Width,4);				tmpLen += 4;			tmp = tmp + 4;
	BH.Height = Y_bitmap;			memcpy(tmp,&BH.Height,4);			tmpLen += 4;			tmp = tmp + 4;
	BH.biPlanes = 1;					memcpy(tmp,&BH.biPlanes,2);			tmpLen += 2;			tmp = tmp + 2;
	BH.biBitCount = 16;				memcpy(tmp,&BH.biBitCount,2);		tmpLen += 2;			tmp = tmp + 2;
	BH.biCompression = 0;			memcpy(tmp,&BH.biCompression,4);		tmpLen += 4;			tmp = tmp + 4;
	BH.biSizeImage = X_bitmap*Y_bitmap*2;memcpy(tmp,&BH.biSizeImage,4);	tmpLen += 4;			tmp = tmp + 4;
	BH.biXPelsPerMeter = 0xB40;		memcpy(tmp,&BH.biXPelsPerMeter,4);	tmpLen += 4;			tmp = tmp + 4;
	BH.biYPelsPerMeter = 0xB40;		memcpy(tmp,&BH.biYPelsPerMeter,4);	tmpLen += 4;			tmp = tmp + 4;
	BH.biClrUsed = 0;				memcpy(tmp,&BH.biClrUsed,4);			tmpLen += 4;			tmp = tmp + 4;
	BH.biClrImportant = 0;			memcpy(tmp,&BH.biClrImportant,4);	tmpLen += 4;			tmp = tmp + 4;

	printf("Writing bitmap ...\n");
	im_loc_bytes=(U32)im_buffer+((U32)Y_bitmap-1)*X_bitmap*4;

	for (y=0; y<Y_bitmap; y++)
	{
		for (x=0;x<X_bitmap;x++)
		{
			pixel=(RGB *)im_loc_bytes;
			t = MY_RGB(pixel->R, pixel->G, pixel->B, 0);
			memcpy(tmp,(U8 *)&t,2);
			tmpLen += 2;
			tmp = tmp + 2;
			im_loc_bytes+=4;
		}
		for (i=0;i<nr_fillingbytes;i++)
		{
			memcpy(tmp,&zero_byte,2);
			tmpLen += 2;
			tmp = tmp + 2;
		}
		im_loc_bytes-=2L*X_bitmap*4;
	}
	*bmpLen = tmpLen;
	
	printf("Done.\n");
	return 0;
}


// Used markers:
#define SOI 0xD8
#define EOI 0xD9
#define APP0 0xE0
#define SOF 0xC0
#define DQT 0xDB
#define DHT 0xC4
#define SOS 0xDA
#define DRI 0xDD
#define COM 0xFE

static U8 *buf; // the buffer we use for storing the entire JPG file
static U8 bp; //current U8
static U16 wp; //current U16
static U32 byte_pos; // current U8 position
#define BYTE_p(i) bp=buf[(i)++]
#define WORD_p(i) wp=(((U16)(buf[(i)]))<<8) + buf[(i)+1]; (i)+=2

// U16 X_image_size,Y_image_size; // X,Y sizes of the image
static U16 X_round,Y_round; // The dimensions rounded to multiple of Hmax*8 (X_round)
			  // and Ymax*8 (Y_round)

static U8 *im_buffer; // RGBA image buffer
static U32 X_image_bytes; // size in bytes of 1 line of the image = X_round * 4
static U32 y_inc_value ; // 32*X_round; // used by decode_MCU_1x2,2x1,2x2

U8 YH,YV,CbH,CbV,CrH,CrV; // sampling factors (horizontal and vertical) for Y,Cb,Cr
static U16 Hmax,Vmax;

static U8 zigzag[64]={ 0, 1, 5, 6,14,15,27,28,
				  2, 4, 7,13,16,26,29,42,
				  3, 8,12,17,25,30,41,43,
				  9,11,18,24,31,40,44,53,
				 10,19,23,32,39,45,52,54,
				 20,22,33,38,46,51,55,60,
				 21,34,37,47,50,56,59,61,
				 35,36,48,49,57,58,62,63 };
typedef struct {
   U8 Length[17];  // k =1-16 ; L[k] indicates the number of Huffman codes of length k
   U16 minor_code[17];  // indicates the value of the smallest Huffman code of length k
   U16 major_code[17];  // similar, but the highest code
   U8 V[65536];  // V[k][j] = Value associated to the j-th Huffman code of length k
	// High nibble = nr of previous 0 coefficients
	// Low nibble = size (in bits) of the coefficient which will be taken from the data stream
} Huffman_table;

static float *QT[4]; // quantization tables, no more than 4 quantization tables (QT[0..3])
static Huffman_table HTDC[4]; //DC huffman tables , no more than 4 (0..3)
static Huffman_table HTAC[4]; //AC huffman tables                  (0..3)

static U8 YQ_nr,CbQ_nr,CrQ_nr; // quantization table number for Y, Cb, Cr
static U8 YDC_nr,CbDC_nr,CrDC_nr; // DC Huffman table number for Y,Cb, Cr
static U8 YAC_nr,CbAC_nr,CrAC_nr; // AC Huffman table number for Y,Cb, Cr

static U8 Restart_markers; // if 1 => Restart markers on , 0 => no restart markers
static U16 MCU_restart; //Restart markers appears every MCU_restart MCU blocks
typedef void (*decode_MCU_func)(U32);


static S16 DCY, DCCb, DCCr; // Coeficientii DC pentru Y,Cb,Cr
static S16 DCT_coeff[64]; // Current DCT_coefficients
static U8 Y[64],Cb[64],Cr[64]; // Y, Cb, Cr of the current 8x8 block for the 1x1 case
static U8 Y_1[64],Y_2[64],Y_3[64],Y_4[64];
static U8 tab_1[64],tab_2[64],tab_3[64],tab_4[64]; // tabelele de supraesantionare pt cele 4 blocuri

static S16 Cr_tab[256],Cb_tab[256]; // Precalculated Cr, Cb tables
static S16 Cr_Cb_green_tab[65536];

// Initial conditions:
// byte_pos = start position in the Huffman coded segment
// WORD_get(w1); WORD_get(w2);wordval=w1;

static U8 d_k=0;  // Bit displacement in memory, relative to the offset of w1
			 // it's always <16
static U16 w1,w2; // w1 = First U16 in memory; w2 = Second U16
static U32 wordval ; // the actual used value in Huffman decoding.
static U32 mask[17];
static S16 neg_pow2[17]={0,-1,-3,-7,-15,-31,-63,-127,-255,-511,-1023,-2047,-4095,-8191,-16383,-32767};
//static U32 start_neg_pow2=(U32)neg_pow2;

static int shift_temp;
#define RIGHT_SHIFT(x,shft)  \
	((shift_temp = (x)) < 0 ? \
	 (shift_temp >> (shft)) | ((~(0L)) << (32-(shft))) : \
	 (shift_temp >> (shft)))
#define DESCALE(x,n)  RIGHT_SHIFT((x) + (1L << ((n)-1)), n)
#define RANGE_MASK 1023L
static U8 *rlimit_table;

/* Allocate and fill in the sample_range_limit table */
void prepare_range_limit_table()
{
	int j;
	rlimit_table = (U8 *)malloc(5 * 256L + 128) ;
	/* First segment of "simple" table: limit[x] = 0 for x < 0 */
	memset((void *)rlimit_table,0,256);
	rlimit_table += 256;	/* allow negative subscripts of simple table */
	/* Main part of "simple" table: limit[x] = x */
	for (j = 0; j < 256; j++) 
		rlimit_table[j] = j;
	/* End of simple table, rest of first half of post-IDCT table */
	for (j = 256; j < 640; j++) 
		rlimit_table[j] = 255;
	/* Second half of post-IDCT table */
	memset((void *)(rlimit_table + 640),0,384);
	for (j = 0; j < 128 ; j++) 
		rlimit_table[j+1024] = j;
}


U16 lookKbits(U8 k)
{
    return (wordval >> (16 - k));
}

//两个字节组成一个字变量
U16 WORD_hi_lo(U8 byte_high,U8 byte_low)
{
    U16 var = 0;

    var |= byte_high;
    var = var << 8;
    var |= byte_low;

    return var;
}

// k>0 always
// Takes k bits out of the BIT stream (wordval), and makes them a signed value
S16 get_svalue(U8 k)
{
	unsigned long wordtemp;
    signed short int ret;

    wordtemp = wordval<<k;
    wordtemp>>=16;

    k--;

    if((wordtemp>>k)&0x00000001)
		return wordtemp;
    else
	{
       	 k++;
       	 ret = wordtemp+neg_pow2[k];
       	 return ret;
    }
}

void skipKbits(U8 k)
{
	U8 b_high,b_low;
	
	d_k+=k;
	if (d_k>=16) 
	{ 
		d_k-=16;
		w1=w2;
		// Get the next U16 in w2
		BYTE_p(byte_pos);
		if (bp!=0xFF) 
			b_high=bp;
		else 
		{
			if (buf[byte_pos]==0) 
				byte_pos++; //skip 00
			else 
				byte_pos--; // stop byte_pos pe restart marker
			b_high=0xFF;
		}
		BYTE_p(byte_pos);
		if (bp!=0xFF) 
			b_low=bp;
		else 
		{
			if (buf[byte_pos]==0) 
				byte_pos++; //skip 00
			else 
				byte_pos--; // stop byte_pos pe restart marker
			b_low=0xFF;
		}
		w2=WORD_hi_lo(b_high,b_low);
	}

	wordval = ((U32)(w1)<<16) + w2;
	wordval <<= d_k;
	wordval >>= 16;
}

S16 getKbits(U8 k)
{
	S16 signed_wordvalue;
	signed_wordvalue=get_svalue(k);
	skipKbits(k);
	return signed_wordvalue;
}

void calculate_mask()
{
	U8 k;
	U32 tmpdv;
	for (k=0;k<=16;k++) 
	{
		tmpdv=0x10000;
		mask[k]=(tmpdv>>k)-1; //precalculated bit mask
	} 
}

void init_QT()
{
	U8 i;
	for (i=0;i<=3;i++) 
		QT[i]=(float *)malloc(sizeof(float)*64);
}

void load_quant_table(float *quant_table)
{
	float scalefactor[8]={1.0f, 1.387039845f, 1.306562965f, 1.175875602f,
						1.0f, 0.785694958f, 0.541196100f, 0.275899379f};
	U8 j,row,col;
	
	for (j=0;j<=63;j++) 
		quant_table[j]=buf[byte_pos+zigzag[j]];
	j=0;
	for (row=0;row<=7;row++)
		for (col=0;col<=7;col++) 
		{
			quant_table[j]*=scalefactor[row]*scalefactor[col];
			j++;
		}
	byte_pos+=64;
}

void load_Huffman_table(Huffman_table *HT)
{
	U8 k,j;
	U32 code;

	for (j=1;j<=16;j++) 
	{
		BYTE_p(byte_pos);
		HT->Length[j]=bp;
	}
	
	for (k=1;k<=16;k++)
		for (j=0;j<HT->Length[k];j++) 
		{
			BYTE_p(byte_pos);
			HT->V[WORD_hi_lo(k,j)]=bp;
		}

	code=0;
	for (k=1;k<=16;k++) 
	{
		HT->minor_code[k] = (U16)code;
		for (j=1;j<=HT->Length[k];j++) 
			code++;
		HT->major_code[k]=(U16)(code-1);
		code*=2;
		if (HT->Length[k]==0) 
		{
			HT->minor_code[k]=0xFFFF;
			HT->major_code[k]=0;
		}
	}
}

// Process one data unit. A data unit = 64 DCT coefficients
// Data is decompressed by Huffman decoding, then the array is dezigzag-ed
// The result is a 64 DCT coefficients array: DCT_coeff
void process_Huffman_data_unit(U8 DC_nr, U8 AC_nr,S16 *previous_DC)
{
	U8 nr,k,j,EOB_found;
	register U16 tmp_Hcode;
	U8 size_val,count_0;
	U16 *min_code,*maj_code; // min_code[k]=minimum code of length k, maj_code[k]=similar but maximum
	U16 *max_val, *min_val;
	U8 *huff_values;
	S16 DCT_tcoeff[64];
	U8 byte_temp;

	// Start Huffman decoding
	// First the DC coefficient decoding
	min_code=HTDC[DC_nr].minor_code;
	maj_code=HTDC[DC_nr].major_code;
	huff_values=HTDC[DC_nr].V;

	for (nr = 0; nr < 64 ; nr++) DCT_tcoeff[nr] = 0; //Initialize DCT_tcoeff

	nr=0;// DC coefficient

	min_val = &min_code[1]; max_val = &maj_code[1];
	for (k=1;k<=16;k++) 
	{
		tmp_Hcode=lookKbits(k);
		//max_val = &maj_code[k]; min_val = &min_code[k];
		if ( (tmp_Hcode<=*max_val)&&(tmp_Hcode>=*min_val) ) //Found a valid Huffman code
		{
			skipKbits(k);
			size_val=huff_values[WORD_hi_lo(k,(U8)(tmp_Hcode-*min_val))];
			if (size_val==0) 
				DCT_tcoeff[0]=*previous_DC;
			else 
			{
				DCT_tcoeff[0]=*previous_DC+getKbits(size_val);
				*previous_DC=DCT_tcoeff[0];
			}
		break;
	 	}
		min_val++; max_val++;
	}

	// Second, AC coefficient decoding
	min_code=HTAC[AC_nr].minor_code;
	maj_code=HTAC[AC_nr].major_code;
	huff_values=HTAC[AC_nr].V;

	nr=1; // AC coefficient
	EOB_found=0;
	while ( (nr<=63)&&(!EOB_found) )
	{
		max_val = &maj_code[1]; min_val =&min_code[1];
		for (k=1;k<=16;k++)
		{
			tmp_Hcode=lookKbits(k);
			//max_val = &maj_code[k]; &min_val = min_code[k];
			if ( (tmp_Hcode<=*max_val)&&(tmp_Hcode>=*min_val) )
			{
				skipKbits(k);
				byte_temp=huff_values[WORD_hi_lo(k,(U8)(tmp_Hcode-*min_val))];
				size_val=byte_temp&0xF;
				count_0=byte_temp>>4;
				if (size_val==0) 
				{
					if (count_0==0) 
						EOB_found=1;
					else if (count_0==0xF) 
						nr+=16; //skip 16 zeroes
				}
				else
				{
					nr+=count_0; //skip count_0 zeroes
					DCT_tcoeff[nr++]=getKbits(size_val);
				}
				break;
			}
			min_val++; max_val++;
		}
		if (k>16) 
			nr++;  // This should not occur
	}
	for (j=0;j<=63;j++) 
		DCT_coeff[j]=DCT_tcoeff[zigzag[j]]; // Et, voila ... DCT_coeff
}

// Fast float IDCT transform
void IDCT_transform(S16 *incoeff,U8 *outcoeff,U8 Q_nr)
{
	U8 x;
	S8 y;
	S16 *inptr;
	U8 *outptr;
	float workspace[64];
	float *wsptr;//Workspace pointer
	float *quantptr; // Quantization table pointer
	float dcval;
	float tmp0,tmp1,tmp2,tmp3,tmp4,tmp5,tmp6,tmp7;
	float tmp10,tmp11,tmp12,tmp13;
	float z5,z10,z11,z12,z13;
	U8 *range_limit=rlimit_table+128;
	// Pass 1: process COLUMNS from input and store into work array.
	wsptr=workspace;
	inptr=incoeff;
	quantptr=QT[Q_nr];
	for (y=0;y<=7;y++)
	{
		if( (inptr[8]|inptr[16]|inptr[24]|inptr[32]|inptr[40]|inptr[48]|inptr[56])==0)
		{
			// AC terms all zero (in a column)
			dcval=inptr[0]*quantptr[0];
			wsptr[0]  = dcval;
			wsptr[8]  = dcval;
			wsptr[16] = dcval;
			wsptr[24] = dcval;
			wsptr[32] = dcval;
			wsptr[40] = dcval;
			wsptr[48] = dcval;
			wsptr[56] = dcval;
			inptr++;quantptr++;wsptr++;//advance pointers to next column
			continue ;
		}
		//Even part
		tmp0 = inptr[0] *quantptr[0];
		tmp1 = inptr[16]*quantptr[16];
		tmp2 = inptr[32]*quantptr[32];
		tmp3 = inptr[48]*quantptr[48];

		tmp10 = tmp0 + tmp2;// phase 3
		tmp11 = tmp0 - tmp2;

		tmp13 = tmp1 + tmp3;// phases 5-3
		tmp12 = (tmp1 - tmp3) * 1.414213562f - tmp13; // 2*c4

		tmp0 = tmp10 + tmp13;// phase 2
		tmp3 = tmp10 - tmp13;
		tmp1 = tmp11 + tmp12;
		tmp2 = tmp11 - tmp12;

		// Odd part
		tmp4 = inptr[8] *quantptr[8];
		tmp5 = inptr[24]*quantptr[24];
		tmp6 = inptr[40]*quantptr[40];
		tmp7 = inptr[56]*quantptr[56];

		z13 = tmp6 + tmp5;// phase 6
		z10 = tmp6 - tmp5;
		z11 = tmp4 + tmp7;
		z12 = tmp4 - tmp7;

		tmp7 = z11 + z13;// phase 5
		tmp11= (z11 - z13) * 1.414213562f; // 2*c4

		z5 = (z10 + z12) * 1.847759065f; // 2*c2
		tmp10 = 1.082392200f * z12 - z5; // 2*(c2-c6)
		tmp12 = -2.613125930f * z10 + z5;// -2*(c2+c6)

		tmp6 = tmp12 - tmp7;// phase 2
		tmp5 = tmp11 - tmp6;
		tmp4 = tmp10 + tmp5;

		wsptr[0]  = tmp0 + tmp7;
		wsptr[56] = tmp0 - tmp7;
		wsptr[8]  = tmp1 + tmp6;
		wsptr[48] = tmp1 - tmp6;
		wsptr[16] = tmp2 + tmp5;
		wsptr[40] = tmp2 - tmp5;
		wsptr[32] = tmp3 + tmp4;
		wsptr[24] = tmp3 - tmp4;
		inptr++;
		quantptr++;
		wsptr++;//advance pointers to the next column
	}

	//  Pass 2: process ROWS from work array, store into output array.
	// Note that we must descale the results by a factor of 8 = 2^3
	outptr=outcoeff;
	wsptr=workspace;
	for (x=0;x<=7;x++)
	{
		// Even part
		tmp10 = wsptr[0] + wsptr[4];
		tmp11 = wsptr[0] - wsptr[4];

		tmp13 = wsptr[2] + wsptr[6];
		tmp12 =(wsptr[2] - wsptr[6]) * 1.414213562f - tmp13;

		tmp0 = tmp10 + tmp13;
		tmp3 = tmp10 - tmp13;
		tmp1 = tmp11 + tmp12;
		tmp2 = tmp11 - tmp12;

		// Odd part
		z13 = wsptr[5] + wsptr[3];
		z10 = wsptr[5] - wsptr[3];
		z11 = wsptr[1] + wsptr[7];
		z12 = wsptr[1] - wsptr[7];

		tmp7 = z11 + z13;
		tmp11= (z11 - z13) * 1.414213562f;

		z5 = (z10 + z12) * 1.847759065f; // 2*c2
		tmp10 = 1.082392200f * z12 - z5;  // 2*(c2-c6)
		tmp12 = -2.613125930f * z10 + z5; // -2*(c2+c6)

		tmp6 = tmp12 - tmp7;
		tmp5 = tmp11 - tmp6;
		tmp4 = tmp10 + tmp5;

		// Final output stage: scale down by a factor of 8
		outptr[0] = range_limit[(DESCALE((int) (tmp0 + tmp7), 3)) & 1023L];
		outptr[7] = range_limit[(DESCALE((int) (tmp0 - tmp7), 3)) & 1023L];
		outptr[1] = range_limit[(DESCALE((int) (tmp1 + tmp6), 3)) & 1023L];
		outptr[6] = range_limit[(DESCALE((int) (tmp1 - tmp6), 3)) & 1023L];
		outptr[2] = range_limit[(DESCALE((int) (tmp2 + tmp5), 3)) & 1023L];
		outptr[5] = range_limit[(DESCALE((int) (tmp2 - tmp5), 3)) & 1023L];
		outptr[4] = range_limit[(DESCALE((int) (tmp3 + tmp4), 3)) & 1023L];
		outptr[3] = range_limit[(DESCALE((int) (tmp3 - tmp4), 3)) & 1023L];

		wsptr+=8;//advance pointer to the next row
		outptr+=8;
	}
}

void precalculate_Cr_Cb_tables()
{
	U16 k;
	U16 Cr_v,Cb_v;
	for (k=0;k<=255;k++) 
		Cr_tab[k]=(S16)((k-128.0)*1.402);
	for (k=0;k<=255;k++) 
		Cb_tab[k]=(S16)((k-128.0)*1.772);

	for (Cr_v=0;Cr_v<=255;Cr_v++)
		for (Cb_v=0;Cb_v<=255;Cb_v++)
			Cr_Cb_green_tab[((U16)(Cr_v)<<8)+Cb_v] = (int)(-0.34414*(Cb_v-128.0)-0.71414*(Cr_v-128.0));
}

// Functia (ca optimizare) poate fi apelata si fara parametrii Y,Cb,Cr
// Stim ca va fi apelata doar in cazul 1x1
void convert_8x8_YCbCr_to_RGB(U8 *Y, U8 *Cb, U8 *Cr, U32 im_loc, U32 X_image_bytes, U8 *im_buffer)
{
	U32 x,y;
	U8 im_nr;
	U8 *Y_val = Y, *Cb_val = Cb, *Cr_val = Cr;
	U8 *ibuffer = im_buffer + im_loc;

	for (y=0;y<8;y++)
	{
		im_nr=0;
		for (x=0;x<8;x++)
		{
			ibuffer[im_nr++] = rlimit_table[*Y_val + Cb_tab[*Cb_val]]; //B
			ibuffer[im_nr++] = rlimit_table[*Y_val + Cr_Cb_green_tab[WORD_hi_lo(*Cr_val,*Cb_val)]]; //G
			ibuffer[im_nr++] = rlimit_table[*Y_val + Cr_tab[*Cr_val]]; // R

			Y_val++; Cb_val++; Cr_val++; im_nr++;
		}
		ibuffer+=X_image_bytes;
	}
}

// Functia (ca optimizare) poate fi apelata si fara parametrii Cb,Cr
void convert_8x8_YCbCr_to_RGB_tab(U8 *Y, U8 *Cb, U8 *Cr, U8 *tab, U32 im_loc, U32 X_image_bytes, U8 *im_buffer)
{
	U32 x,y;
	U8 nr, im_nr;
	U8 Y_val,Cb_val,Cr_val;
	U8 *ibuffer = im_buffer + im_loc;

	nr=0;
	for (y=0;y<8;y++)
	{
		im_nr=0;
		for (x=0;x<8;x++)
		{
			Y_val=Y[nr];
			Cb_val=Cb[tab[nr]]; 
			Cr_val=Cr[tab[nr]]; // reindexare folosind tabelul
			// de supraesantionare precalculat
			ibuffer[im_nr++] = rlimit_table[Y_val + Cb_tab[Cb_val]]; //B
			ibuffer[im_nr++] = rlimit_table[Y_val + Cr_Cb_green_tab[WORD_hi_lo(Cr_val,Cb_val)]]; //G
			ibuffer[im_nr++] = rlimit_table[Y_val + Cr_tab[Cr_val]]; // R
			nr++;
			im_nr++;
		}
		ibuffer+=X_image_bytes;
	}
}

void calculate_tabs()
{
	U8 tab_temp[256];
	U8 x,y;

	// Tabelul de supraesantionare 16x16
	for (y=0;y<16;y++)
		for (x=0;x<16;x++)
			tab_temp[y*16+x] = (y/YV)* 8 + x/YH;

	// Din el derivam tabelele corespunzatoare celor 4 blocuri de 8x8 pixeli
	for (y=0;y<8;y++)
	{
		for (x=0;x<8;x++)
			tab_1[y*8+x]=tab_temp[y*16+x];
		for (x=8;x<16;x++)
			tab_2[y*8+(x-8)]=tab_temp[y*16+x];
	}
	for (y=8;y<16;y++)
	{
		for (x=0;x<8;x++)
			tab_3[(y-8)*8+x]=tab_temp[y*16+x];
		for (x=8;x<16;x++)
			tab_4[(y-8)*8+(x-8)]=tab_temp[y*16+x];
	}
}

int init_JPG_decoding()
{
	byte_pos=0;
	init_QT();
	calculate_mask();
	prepare_range_limit_table();
	precalculate_Cr_Cb_tables();
	return 1; //for future error check
}

int load_JPEG_header(U8 *pJpgData, int jpgLen, U32 *X_image, U32 *Y_image)
{
	U32 length_of_file;
	U8 vers,units;
	U16 Xdensity,Ydensity,Xthumbnail,Ythumbnail;
	U16 length;
	float *qtable;
	U32 old_byte_pos;
	Huffman_table *htable;
	U32 j;
	U8 precision,comp_id,nr_components;
	U8 QT_info,HT_info;
	U8 SOS_found,SOF_found;

	length_of_file=(U32)jpgLen;
	buf=(U8 *)malloc(length_of_file+4);
	if (buf==NULL)
		exit_func("Not enough memory for loading file");
	memcpy(buf,pJpgData,length_of_file);

	if ((buf[0]!=0xFF)||(buf[1]!=SOI)) 
		exit_func("Not a JPG file ?\n");
	if ((buf[2]!=0xFF)||(buf[3]!=APP0)) 
		exit_func("Invalid JPG file.");
	if ( (buf[6]!='J')||(buf[7]!='F')||(buf[8]!='I')||(buf[9]!='F')|| (buf[10]!=0) ) 
		exit_func("Invalid JPG file.");

	init_JPG_decoding();
	byte_pos=11;

	BYTE_p(byte_pos);	vers=bp;
	if (vers!=1) 
		exit_func("JFIF version not supported");
	BYTE_p(byte_pos);
	BYTE_p(byte_pos);	units=bp;
	WORD_p(byte_pos);	Xdensity=wp; 
	WORD_p(byte_pos);	Ydensity=wp;
	BYTE_p(byte_pos);	Xthumbnail=bp;
	BYTE_p(byte_pos);	Ythumbnail=bp;
	if ((Xthumbnail!=0)||(Ythumbnail!=0))
		exit_func(" Cannot process JFIF thumbnailed files\n");
	
	// Start decoding process
	SOS_found=0; 
	SOF_found=0; 
	Restart_markers=0;
	while ((byte_pos<length_of_file)&&!SOS_found)
	{
		BYTE_p(byte_pos);
		if (bp!=0xFF) 
			continue;
		// A marker was found
		BYTE_p(byte_pos);
		switch(bp)
		{
			case DQT: WORD_p(byte_pos);	length=wp; // length of the DQT marker
				for (j=0;j<wp-2;)
				{
					old_byte_pos=byte_pos;
					BYTE_p(byte_pos);	QT_info=bp;
					if ((QT_info>>4)!=0)
						exit_func("16 bit quantization table not supported");
					qtable=QT[QT_info&0xF];
					load_quant_table(qtable);
					j+=byte_pos-old_byte_pos;
				}
				break;
			case DHT: 
				WORD_p(byte_pos); length=wp;
				for (j=0;j<wp-2;)
				{
					old_byte_pos=byte_pos;
					BYTE_p(byte_pos);	HT_info=bp;
					if ((HT_info&0x10)!=0) 
						htable=&HTAC[HT_info&0xF];
					else 
						htable=&HTDC[HT_info&0xF];
					load_Huffman_table(htable);
					j+=byte_pos-old_byte_pos;
				}
				break;
			case COM: 
				WORD_p(byte_pos);	length=wp;
				byte_pos+=wp-2;
				break;
			case DRI: 
				Restart_markers=1;
				WORD_p(byte_pos);	length=wp; //should be = 4
				WORD_p(byte_pos);	MCU_restart=wp;
				if (MCU_restart==0) 
					Restart_markers=0;
				break;
			case SOF: 
				WORD_p(byte_pos);	length=wp; //should be = 8+3*3=17
				BYTE_p(byte_pos);	precision=bp;
				if (precision!=8) 
					exit_func("Only 8 bit precision supported");
				WORD_p(byte_pos);	*Y_image=wp; 
				WORD_p(byte_pos);	*X_image=wp;
				BYTE_p(byte_pos);	nr_components=bp;
				if (nr_components!=3) 
					exit_func("Only truecolor JPGS supported");
				for (j=1;j<=3;j++)
				{
					BYTE_p(byte_pos);	comp_id=bp;
					if ((comp_id==0)||(comp_id>3)) 
						exit_func("Only YCbCr format supported");
					switch (comp_id)
					{
						case 1: // Y
							BYTE_p(byte_pos);	YH=bp>>4;YV=bp&0xF;
							BYTE_p(byte_pos);	YQ_nr=bp;
							break;
						case 2: // Cb
							BYTE_p(byte_pos);	CbH=bp>>4;CbV=bp&0xF;
							BYTE_p(byte_pos);	CbQ_nr=bp;
							break;
						case 3: // Cr
							BYTE_p(byte_pos);	CrH=bp>>4;CrV=bp&0xF;
							BYTE_p(byte_pos);	CrQ_nr=bp;
							break;
					}
				}
				SOF_found=1;
				break;
			case SOS: 
				WORD_p(byte_pos);	length=wp; //should be = 6+3*2=12
				BYTE_p(byte_pos);	nr_components=bp;
				if (nr_components!=3) 
					exit_func("Invalid SOS marker");
				for (j=1;j<=3;j++)
				{
					BYTE_p(byte_pos);	comp_id=bp;
					if ((comp_id==0)||(comp_id>3)) 
						exit_func("Only YCbCr format supported");
					switch (comp_id)
					{
						case 1: // Y
							BYTE_p(byte_pos);	YDC_nr=bp>>4;YAC_nr=bp&0xF;
							break;
						case 2: // Cb
							BYTE_p(byte_pos);	CbDC_nr=bp>>4;CbAC_nr=bp&0xF;
							break;
						case 3: // Cr
							BYTE_p(byte_pos);	CrDC_nr=bp>>4;CrAC_nr=bp&0xF;
							break;
					}
				}
				BYTE_p(byte_pos); BYTE_p(byte_pos); BYTE_p(byte_pos); // Skip 3 bytes
				SOS_found=1;
				break;
			case 0xFF:
				break; // do nothing for 0xFFFF, sequence of consecutive 0xFF are for
			// filling purposes and should be ignored
			default:  
				WORD_p(byte_pos);	length=wp;
				byte_pos+=wp-2; //skip unknown marker
				break;
		}
	}
	if (!SOS_found)	exit_func("Invalid JPG file. No SOS marker found.");
	if (!SOF_found)	exit_func("Progressive JPEGs not supported");

	if ((CbH>YH)||(CrH>YH))	exit_func("Vertical sampling factor for Y should be >= sampling factor for Cb,Cr");
	if ((CbV>YV)||(CrV>YV))	exit_func("Horizontal sampling factor for Y should be >= sampling factor for Cb,Cr");

	if ((CbH>=2)||(CbV>=2))	exit_func("Cb sampling factors should be = 1");
	if ((CrV>=2)||(CrV>=2))	exit_func("Cr sampling factors should be = 1");

	// Restricting sampling factors for Y,Cb,Cr should give us 4 possible cases for sampling factors
	// YHxYV,CbHxCbV,CrHxCrV: 2x2,1x1,1x1;  1x2,1x1,1x1; 2x1,1x1,1x1;
	// and 1x1,1x1,1x1 = no upsampling needed
	Hmax=YH,Vmax=YV;
	if ( *X_image%(Hmax*8)==0) 
		X_round=*X_image; // X_round = Multiple of Hmax*8
	else 
		X_round=(*X_image/(Hmax*8)+1)*(Hmax*8);
	if ( *Y_image%(Vmax*8)==0) 
		Y_round=*Y_image; // Y_round = Multiple of Vmax*8
	else 
		Y_round=(*Y_image/(Vmax*8)+1)*(Vmax*8);

	im_buffer=(U8 *)malloc(X_round*Y_round*4);
	if (im_buffer==NULL) 
		exit_func("Not enough memory for storing the JPEG image");

	return 1;
}

// byte_pos  = pozitionat pe restart marker
void resync()
{
	byte_pos+=2;
	BYTE_p(byte_pos);
	if (bp==0xFF)
		byte_pos++; // skip 00
	w1=WORD_hi_lo(bp, 0);
	BYTE_p(byte_pos);
	if (bp==0xFF)
		byte_pos++; // skip 00
	w1+=bp;
	BYTE_p(byte_pos);
	if (bp==0xFF)
		byte_pos++; // skip 00
	w2=WORD_hi_lo(bp, 0);
	BYTE_p(byte_pos);
	if (bp==0xFF)
		byte_pos++; // skip 00
	w2+=bp;
	wordval=w1; 
	d_k=0; // Reinit bitstream decoding
	DCY=0; 
	DCCb=0; 
	DCCr=0; // Init DC coefficients
}

void decode_MCU_1x1(U32 im_loc)
{
	// Y
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y,YQ_nr);
	// Cb
	process_Huffman_data_unit(CbDC_nr,CbAC_nr,&DCCb);
	IDCT_transform(DCT_coeff,Cb,CbQ_nr);
	// Cr
	process_Huffman_data_unit(CrDC_nr,CrAC_nr,&DCCr);
	IDCT_transform(DCT_coeff,Cr,CrQ_nr);

	convert_8x8_YCbCr_to_RGB(Y,Cb,Cr,im_loc,X_image_bytes,im_buffer);
}
void decode_MCU_2x1(U32 im_loc)
{
	// Y
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y_1,YQ_nr);
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y_2,YQ_nr);
	// Cb
	process_Huffman_data_unit(CbDC_nr,CbAC_nr,&DCCb);
	IDCT_transform(DCT_coeff,Cb,CbQ_nr);
	// Cr
	process_Huffman_data_unit(CrDC_nr,CrAC_nr,&DCCr);
	IDCT_transform(DCT_coeff,Cr,CrQ_nr);

	convert_8x8_YCbCr_to_RGB_tab(Y_1,Cb,Cr,tab_1,im_loc,X_image_bytes,im_buffer);
	convert_8x8_YCbCr_to_RGB_tab(Y_2,Cb,Cr,tab_2,im_loc+32,X_image_bytes,im_buffer);
}

void decode_MCU_2x2(U32 im_loc)
{
	// Y
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y_1,YQ_nr);
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y_2,YQ_nr);
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y_3,YQ_nr);
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y_4,YQ_nr);
	// Cb
	process_Huffman_data_unit(CbDC_nr,CbAC_nr,&DCCb);
	IDCT_transform(DCT_coeff,Cb,CbQ_nr);
	// Cr
	process_Huffman_data_unit(CrDC_nr,CrAC_nr,&DCCr);
	IDCT_transform(DCT_coeff,Cr,CrQ_nr);

	convert_8x8_YCbCr_to_RGB_tab(Y_1,Cb,Cr,tab_1,im_loc,X_image_bytes,im_buffer);
	convert_8x8_YCbCr_to_RGB_tab(Y_2,Cb,Cr,tab_2,im_loc+32,X_image_bytes,im_buffer);
	convert_8x8_YCbCr_to_RGB_tab(Y_3,Cb,Cr,tab_3,im_loc+y_inc_value,X_image_bytes,im_buffer);
	convert_8x8_YCbCr_to_RGB_tab(Y_4,Cb,Cr,tab_4,im_loc+y_inc_value+32,X_image_bytes,im_buffer);
}

void decode_MCU_1x2(U32 im_loc)
{
	// Y
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y_1,YQ_nr);
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y_2,YQ_nr);
	// Cb
	process_Huffman_data_unit(CbDC_nr,CbAC_nr,&DCCb);
	IDCT_transform(DCT_coeff,Cb,CbQ_nr);
	// Cr
	process_Huffman_data_unit(CrDC_nr,CrAC_nr,&DCCr);
	IDCT_transform(DCT_coeff,Cr,CrQ_nr);

	convert_8x8_YCbCr_to_RGB_tab(Y_1,Cb,Cr,tab_1,im_loc,X_image_bytes,im_buffer);
	convert_8x8_YCbCr_to_RGB_tab(Y_2,Cb,Cr,tab_3,im_loc+y_inc_value,X_image_bytes,im_buffer);
}

void decode_JPEG_image()
{
	decode_MCU_func decode_MCU;
	U16 x_mcu_cnt,y_mcu_cnt;
	U32 nr_mcu;
	U16 X_MCU_nr,Y_MCU_nr; // Nr de MCU-uri
	U32 MCU_dim_x; //dimensiunea in bufferul imagine a unui MCU pe axa x
	U32 im_loc_inc; // = 7 * X_round * 4 sau 15*X_round*4;
	U32 im_loc; //locatia in bufferul imagine

	byte_pos -= 2;
	resync();

	y_inc_value = 32*X_round;
	calculate_tabs(); // Calcul tabele de supraesantionare, tinand cont de YH si YV

	if ((YH==1)&&(YV==1)) 
		decode_MCU=decode_MCU_1x1;
	else 
	{
		if (YH==2)
		{
			if (YV==2) 
				decode_MCU=decode_MCU_2x2;
			else 
				decode_MCU=decode_MCU_2x1;
		}
		else 
			decode_MCU=decode_MCU_1x2;
	}
	MCU_dim_x=Hmax*8*4;
	Y_MCU_nr=Y_round/(Vmax*8); // nr of MCUs on Y axis
	X_MCU_nr=X_round/(Hmax*8); // nr of MCUs on X axis

	X_image_bytes=X_round*4;
	im_loc_inc = (Vmax*8-1) * X_image_bytes;
	nr_mcu=0; 
	im_loc=0; // memory location of the current MCU
	for (y_mcu_cnt=0;y_mcu_cnt<Y_MCU_nr;y_mcu_cnt++)
	{
		for (x_mcu_cnt=0;x_mcu_cnt<X_MCU_nr;x_mcu_cnt++)
		{
			decode_MCU(im_loc);
			if ((Restart_markers)&&((nr_mcu+1)%MCU_restart==0)) 
				resync();
			nr_mcu++;
			im_loc+=MCU_dim_x;
		}
		im_loc+=im_loc_inc;
	}
}

int get_JPEG_buffer(U16 X_image,U16 Y_image, U8 **address_dest_buffer)
{
	U16 y;
	U8 *src_buffer=im_buffer;
	U8 *dest_buffer_start, *dest_buffer;
	U32 src_bytes_per_line=X_round*4;
	U32 dest_bytes_per_line=X_image*4;

	if ((X_round==X_image)&&(Y_round==Y_image))
		*address_dest_buffer=im_buffer;
	else
	{
		dest_buffer_start = (U8 *)malloc(X_image*Y_image*4);
		if (dest_buffer_start==NULL) 
			exit_func("Not enough memory for storing the JPEG image");
		dest_buffer = dest_buffer_start;
		for (y=0;y<Y_image;y++) 
		{
			memcpy(dest_buffer,src_buffer,dest_bytes_per_line);
			src_buffer+=src_bytes_per_line;
			dest_buffer+=dest_bytes_per_line;
		}
		*address_dest_buffer=dest_buffer_start;
		free(im_buffer);
		im_buffer = NULL;
	}
	// release the buffer which contains the JPG file
	free(buf);
	buf = NULL;
	return 1;
}

int _transformJpg2Bmp(unsigned char *pJpgData, int jpgLen,unsigned char *pBmpData, int *bmpLen)
{
	if (!pJpgData || !pBmpData || !bmpLen)
	{
		printf("JPG or BMP data is NULL\n");
		return -1;
	}

	unsigned int X_image; 
	unsigned int Y_image;
	unsigned char *our_image_buffer;
	int i = 0;
	
	if (!load_JPEG_header(pJpgData, jpgLen, &X_image, &Y_image)) 
	{
		printf("load_JPEG_header Err!\n");
		return -1;
	}

	printf(" X_image = %d\n", X_image);
 	printf(" Y_image = %d\n", Y_image);

	decode_JPEG_image();

	if (!get_JPEG_buffer(X_image, Y_image, &our_image_buffer)) 
	{
		printf("get_JPEG_buffer Err!\n");
		return -1;
	}

	write_buf_to_BMP(our_image_buffer, X_image, Y_image, pBmpData, bmpLen);

	if ((X_round!=X_image)||(Y_round!=Y_image))
	{
		if(our_image_buffer != NULL)
		{
			free(our_image_buffer);
			our_image_buffer = NULL;
		}
	}
	if(rlimit_table != NULL)
	{
		free(rlimit_table);
		rlimit_table = NULL;
	}
	for (i=0;i<=3;i++)
	{
		if(QT[i] != NULL)
		{
			free(QT[i]);
			QT[i] = NULL;
		}
	}
	if(im_buffer != NULL)
	{
		free(im_buffer);
		im_buffer = NULL;
	}
	if(buf != NULL)
	{
		free(buf);
		buf = NULL;
	}

	return 0;
}

