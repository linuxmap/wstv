
#ifndef _TUYA_RING_BUFFER_
#define _TUYA_RING_BUFFER_

#ifdef __cplusplus
extern "C" {
#endif


#include "defines.h"
#include "tuya_cloud_types.h"
#include "tuya_ipc_media.h"
#include "tuya_cloud_error_code.h"


#define DEFAULT_VIDEO_BITRATE           (256*1024) //2Mbps=256KB/s
#define DEFAULT_VIDEO_BUFFER_POOL_SIZE  (DEFAULT_VIDEO_BITRATE*4) //4s buffer
#define DEFAULT_VIDEO_FRAME_RATE        20
#define DEFAULT_VIDEO_GOP_FRAMES        (DEFAULT_VIDEO_FRAME_RATE*4)
#define MAX_BUFFER_NODE_NUM             DEFAULT_VIDEO_GOP_FRAMES

typedef enum
{
    E_USER_STREAM_STORAGE = 0,
    E_USER_COULD_STORAGE,
    E_USER_TUTK_P2P,
    E_USER_ECHO_SHOW,
    E_USER_PREVIEW_3,
    E_USER_NUM_MAX
}USER_INDEX_E;

typedef struct 
{
    UINT index;
    MEDIA_FRAME_TYPE_E type;
    UCHAR *rawData;
    UINT size;
    UINT64 pts;
    UINT64 timestamp;
    UINT seqNo;
    UCHAR *extraData;
    UINT extraSize;
}Ring_Buffer_Node_S;

/* 初始化一个 ring buffer
channel: buffer ring支持多个，针对IPC可能会存在的主码流、子码流、三码流等场景，
bufferSize: 本路码流缓存数据量的大小，bytes。为0时使用默认大小。
pConfig: 音视频编码等信息
*/
OPERATE_RET tuya_ipc_ring_buffer_init(CHANNEL_E channel, UINT bufferSize);

#if 0
/* 销毁一个 ring buffer*/
OPERATE_RET tuya_ipc_ring_buffer_destory(CHANNEL_E channel);
#endif
/* 往ring buffer中追加新的数据
*/
OPERATE_RET tuya_ipc_ring_buffer_append_data(CHANNEL_E channel, UCHAR *addr, UINT size, MEDIA_FRAME_TYPE_E type, UINT64 pts);

/* 从ringbuffer中取数据。 由userIndex区分同一份环形缓存的不同用户，各位维护取数据的状态。
当取数据落后写数据超过阈值时，取数据的接口会自动跳帧取到最新的I帧或最新的音频帧。
*/
Ring_Buffer_Node_S *tuya_ipc_ring_buffer_get_video_frame(CHANNEL_E channel, USER_INDEX_E userIndex, BOOL isRetry);
Ring_Buffer_Node_S *tuya_ipc_ring_buffer_get_audio_frame(CHANNEL_E channel, USER_INDEX_E userIndex, BOOL isRetry);
INT tuya_ipc_ring_buffer_update_user_info_to_pre_video_frames(CHANNEL_E channel, USER_INDEX_E userIndex, UINT frameNum);
INT tuya_ipc_ring_buffer_update_user_info_to_pre_audio_frames(CHANNEL_E channel, USER_INDEX_E userIndex, UINT frameNum);

#ifdef __cplusplus
}
#endif

#endif  /*_TUYA_RING_BUFFER_*/


