#ifndef BLS_H
#define BLS_H

#include "bls_type.h"

#define BLS_STR_LEN		256

typedef struct
{
	char access_token[BLS_STR_LEN];
	char stream_ID[BLS_STR_LEN];
	
}BLSInfoType;

int BLSBuildStart();
void BLSBuildEnd();
int BLSSendAVCSH(BLS_SPS_PPS* p);
int BLSSendVideoPacket(unsigned char* data, int size, unsigned long long timestamp, int is_key);
int BLSSendAudioPacket(unsigned char* data, int size, unsigned long long timestamp);
int BLSInfoSave(BLSInfoType* info);



#endif
