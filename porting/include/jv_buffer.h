/*
 * jv_buffer.h
 *
 *  Created on: 2016-4-15
 *      Author: Administrator
 */

#ifndef JV_BUFFER_H_
#define JV_BUFFER_H_
#ifdef __cplusplus
extern "C" {
#endif

typedef void * BUF_HANDLE;

BUF_HANDLE jv_buf_create(int totalSize,int maxBufCount);
int jv_buf_readFrame(BUF_HANDLE handle, unsigned char *buf, int len, unsigned long long *timestamp, int * frameType);
int jv_buf_writeFrame(BUF_HANDLE handle, unsigned char *buf, int len, unsigned long long timestamp, int frameType);

#ifdef __cplusplus
}
#endif


#endif /* JV_BUFFER_H_ */
