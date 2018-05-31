/**
 *@file jv_osddrv.h osd device driver
 * define the interface of osd drawing
 *@author Liu Fengxiang
 */

#ifndef _JV_OSDDRV_H_
#define _JV_OSDDRV_H_
#include "jv_common.h"


/**
 *@brief color types definition
 *
 */
typedef enum {
	OSDDRV_COLOR_TYPE_8_CLUT8 = 0,
	OSDDRV_COLOR_TYPE_16_ARGB1555,
	OSDDRV_COLOR_TYPE_16_ARGB4444,
	OSDDRV_COLOR_TYPE_16_RGB565,
	OSDDRV_COLOR_TYPE_24_RGB888,
	OSDDRV_COLOR_TYPE_32_ARGB8888,
	OSDDRV_COLOR_TYPE_4_UNKNOWN,
	OSDDRV_COLOR_TYPE_MAX
}jv_osddrv_color_type_t;

typedef struct {
	jv_osddrv_color_type_t type;
	unsigned char a;
	unsigned char r;
	unsigned char g;
	unsigned char b;
}jv_osddrv_color_t;

/**
 *@brief osd region attr
 *
 */
typedef struct{
	jv_osddrv_color_type_t type;
	RECT rect;
	BOOL jv_osddrv_inv_col_en;	//是否打开反色
}jv_osddrv_region_attr;

typedef struct{
	jv_osddrv_color_type_t type;	///< color type of the osd 
	RECT rect;				///< region of the osd
	unsigned char *buffer;		///< memory address to draw
	int pitch;					///< width of the osd in byte, It maybe not the value of rect.w
	int len;					///< length of the maping data
	int channelid;			///< channelid
}jv_osddrv_mapping_info_t;

/**
 *@brief do initialize of osd driver
 *初始化
 *@return 0 if success.
 *
 */
int jv_osddrv_init(void);

/**
 *@brief do de-initialize of osd driver
 *结束
 *@return 0 if success.
 *
 */
int jv_osddrv_deinit(void);

/**
 *@brief 每个通道可以创建的最多区域数
 */
int jv_osddrv_max_region(void);


/**
 *@brief create osd region 
 *@param channelid The id of the channel.
 *@param attr osd region attribute fro create a osd region
 *@param jv_osddrv_inv_col_en 是否开启反色
 *@retval <0 error happened
 *@retval >=0 handle of osd
 *
 */
int jv_osddrv_create(int channelid, jv_osddrv_region_attr *attr);

/**
 *@brief destroy osd region 
 *@param handle It is the retval of #jv_osddrv_create
 *@retval <0 error happened
 *@retval 0 success
 *
 */
int jv_osddrv_destroy(int handle);

/**
 *@brief clear the draw buffer. erase all
 *
 *@param handle It is the retval of #jv_osddrv_create
 */
int jv_osddrv_clear(int handle);

/**
 *@brief draw bitmap 
 *@param handle It is the retval of #jv_osddrv_create
 *@param rect region to draw 
 *@param data pointer to data buffer, and It's length depend on the colortype of the osd handle
 *@retval <0 error happened
 *@retval 0 success
 *
 */
int jv_osddrv_drawbitmap(int handle, RECT *rect, unsigned char *data);

/**
 *@brief get map address of the osd
 *@note with mapped address, you should #jv_osddrv_flush to enable your drawing
 *@param handle It is the retval of jv_osddrv_create
 *@param map output, the mapping info
 *@retval <0 error happened
 *@retval 0 success
 *
 */
int jv_osddrv_get_mapping(int handle, jv_osddrv_mapping_info_t *map);

/**
 *@brief flush the region
 * sometimes, the osd need to flush to enable the drawing.
 *@param handle It is the retval of jv_osddrv_create
 *@retval <0 error happened
 *@retval 0 success
 *
 */
int jv_osddrv_flush(int handle);

/**
 *@brief color convert
 * convert color from structure to unsigned int value
 *@param color color structure pointer
 *@retval <0 error happened
 *@retval 0 success
 *
 */
unsigned int jv_osddrv_color2uint(jv_osddrv_color_t *color);

/**
 *@brief color convert
 * convert color from unsigned int value to structure 
 *@param value It is the value of the color in unsigned int
 *@param type the type of color to convert
 *@param color output, the structure you wantted
 *@retval <0 error happened
 *@retval 0 success
 *
 */
int jv_osddrv_uint2color(unsigned int value, jv_osddrv_color_type_t type, jv_osddrv_color_t *color);

typedef struct {
	unsigned short clear; //透明色
	unsigned short white;
	unsigned short black;
	unsigned short gray;
}CommonColor_t;

int jv_osddrv_get_common_color(CommonColor_t *cc);

#endif
 

