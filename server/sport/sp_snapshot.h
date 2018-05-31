/*
 * sp_snapshot.h
 *
 *  Created on: 2013Äê12ÔÂ23ÈÕ
 *      Author: lfx  20451250@qq.com
 */
#ifndef SP_SNAPSHOT_H_
#define SP_SNAPSHOT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __SP_SNAP_SHOT_SIZE
{
	int nWith;
	int nHeight;
}stSPSnapSize;

int sp_snapshot(int channelid, const char *fname);

const char* sp_snapshot_get_uri(int channelid);

int sp_snapshort_get_data(int channelid, unsigned char *pData ,unsigned int len, stSPSnapSize *snap);


#ifdef __cplusplus
}
#endif


#endif /* SP_SNAPSHOT_H_ */
