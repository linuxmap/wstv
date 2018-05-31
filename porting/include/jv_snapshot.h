/**
 *@file jv_snapshot.h file about snap shot
 * define the interface of snap shot.
 *@author Liu Fengxiang
 */
#ifndef _JV_SNAPSHOT_H_
#define _JV_SNAPSHOT_H_
#include "jv_common.h"

typedef struct __SNAP_SHOT_SIZE
{
	U32 nWith;
	U32 nHeight;
}stSnapSize;

/**
 *@brief snap shot
 *@param handle handle of the stream
 *@param channelid channel id
 *@param w width of photo wantted
 *@param h height of photo wantted
 *@param quality quality of photo, value in [1,5]
 *@param data buffer for photo data
 *@retval 0 if success
 *@retval <0 if failed. JV_ERR_XXX
 */
int jv_snapshot_get(int channelid, unsigned char *data,unsigned int len);

//可以设置大小，dll中的图片不需要那么大
int jv_snapshot_get_ex(int channelid, unsigned char *pData ,unsigned int len, stSnapSize *snap);

int jv_snapshot_get_def_size(stSnapSize *snap);

void jv_snapshot_done(unsigned char *pData, unsigned int len);

#endif

