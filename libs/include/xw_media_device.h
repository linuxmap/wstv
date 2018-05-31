/*=============================================================================
#
# Author: haitao - haitao-330@sohu.com
#
# Last modified:	2017-11-30 09:17
#
# Filename:		xw_media_device.h
#
# Description: 
#
=============================================================================*/
#ifndef _XW_MEDIA_DEVICE_H_
#define _XW_MEDIA_DEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MAX_PACK_LENGTH (500 * 1024)

typedef void (*ON_DEVICE_RECEIVED)(uint32_t user_id, uint8_t data_type, const uint8_t* data, int data_len);
typedef void (*ON_PHONE_LOGOUT)(uint32_t user_id);

/*
 * return: 0 success
 *         other failed
 */
typedef int (*ON_VERIFY_USER_NAME_AND_PW)(const char* device_user_name, const char* device_user_pw);

//Transfer Data Type
enum {
    DT_I_FRAME = 1,
    DT_P_FRAME,
    DT_O_FRAME,
    DT_AUDIO,
    DT_NORMAL_TXT,
    DT_FILE_HEAD,
    DT_FILE_DATA,
    DT_FILE_TAIL,
    DT_RECORD_DATA,
    DT_RECORD_END,
    //DT_DIRECT_TRANSMIT,
    DT_MAX
};

const char* xw_media_device_version();

/*
 * the First interface of user-call
 */
void xw_media_device_set_current_net_interface_name(const char* name);

/*
 * the Second interface of user-call
 */
int xw_media_device_init(const char* ystno, const char* flash_path, const char* sdcard_path, ON_DEVICE_RECEIVED on_received, ON_PHONE_LOGOUT phone_logout, ON_VERIFY_USER_NAME_AND_PW verify_user_name_and_pw);

/*
 * send reliable data to user_id 
 */
void xw_media_device_send_normal_data(uint32_t user_id, uint8_t* data, int data_len);

/*
 *1. when send real-time media data, the user_id must be NULL 
 *2. when send record media data, user_id must non-NULL
 */
void xw_media_device_push_media_data(uint32_t user_id, uint8_t media_data_type, uint8_t* media_data, int media_data_len, uint64_t timestamp);

void xw_media_device_clear_record_media_data(uint32_t user_id);

/*
 * file_name: full path file name
 *
 * return: >0 local send file handle
 *         -1 open file failed
 *         -2 get file stat failed
 *         -3 the hphone is transfering, unable to again
 *         -4 file path is too longer
 *         -5 file name is null
 *         -6 hphone is legal
 */
int xw_media_device_send_file(uint32_t user_id, const char* file_name);
void xw_media_device_cancel_send_file(int hsend_file);

/*
 * on: 0 close switch
 *     1 open switch
 *     
 */
void xw_media_device_set_search_device_switch(uint8_t on);

#ifdef __cplusplus
}
#endif

#endif

