/** ===========================================================================
* @path $(IPNCPATH)\interface\inc\
*
* @desc
* .
* Copyright (c) Appro Photoelectron Inc.  2008
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied
*
* =========================================================================== */
/**
* @file Msg_Def.h
* @brief Definition of message command, message key, and message type.
*/
#ifndef __MSG_H__
#define __MSG_H__

#if defined (__cplusplus)
extern "C" {
#endif
typedef int  (*Handle_get_ircut_staus_callback_t)(void);

#if 1		//SNS
enum E_LENS_PARAM{
	E_CMD_LENS_INIT=1,				//?μí·init
	E_CMD_LENS_ZOOM,			//?μí·ZOOM	
	E_CMD_LENS_FOCUS,				//?μí·AF
	E_CMD_LENS_PRESET_POS,			//?μí·ZOOM
	E_CMD_LENS_IRCUT,			//?μí·IRCut
	E_CMD_LENS_IRIS,				//?μí·Iris
	E_CMD_LENS_CONFIG,			//?μí·????
	E_CMD_LENS_STATE,			//?μí·×′ì?2é?ˉ
	E_CMD_LENS_DAYNIGHT,			//?μí·×′ì?2é?ˉ

	E_CMD_PTZ_TRANSMIT,	//???ù?òí??÷′?ê?	
	E_CMD_PTZ_TOUR_SET,		//???ù?ò?2o?éè??	
	E_CMD_PTZ_TOUR_START,				//???ù?ò?2o????ˉ	
	E_CMD_PTZ_DIRECTION_CONTROL,		//???ù?ò·??ò????
	E_CMD_PTZ_POS_SET,					//???ù?ò????????éè??
	E_CMD_PTZ_ZONE_SCAN,				//???ù?ò??óòé¨?è
	E_CMD_PTZ_PATTERN_SCAN,			//???ù?ò?¨?ùé¨?è
	E_CMD_PTZ_PRESET,			//???ù?ò?¤??μ?
	E_CMD_LENS_ZOOMRATIO,
	E_CMD_LENS_AFMANUALAUTO,///AF 自动手动模式切换
	E_CMD_LENS_AFRESET,
	E_CMD_LENS_DOMERESUALTAF,
};

#pragma pack(1)

typedef struct {//0x33.éè???μí·ZOOM
	unsigned char bCmdType;
	unsigned char bCmdParam;		//AF?üá?ààDí
	unsigned int nZoomSection;		//ZOOM?üá?ààDí
	unsigned int nFocusPos;		//ZOOM??μ?????
	unsigned int nFocusZeroPos;		//Focus PI point
	unsigned char nNDMin;
	unsigned char nNDMax;	
	unsigned char init;		
}T_LENS_INIT;

typedef struct {//0x33.éè???μí·ZOOM
	unsigned char bCmdType;
	unsigned char bCmdParam;		//AF?üá?ààDí
	unsigned int nSpeed;		//ZOOM?üá?ààDí
	int nZoomSection;		//ZOOM??μ?????
	int nFocusPos;		//ZOOM??μ?????
	unsigned char bResetRouteFlag;	///用于控球导致的场景改变而重新高倍到低倍时定位曲线
	unsigned char bIsDomeMove;
	unsigned char ManuInitMotorFlag;
}T_LENS_ZOOM;

typedef struct {				//0x32.éè???μí·AF
	unsigned char bCmdType;
	unsigned char bCmdParam;		//AF?üá?ààDí
	unsigned char bSide;		//far --2   near---1
	unsigned int nStep;		//AFò??ˉ2?3¤ ·??§1-100￡¨ê??ˉ???1ê±óDD§￡?
}T_LENS_FOCUS;

typedef struct {//0x34.éè???μí·IRCut1|?ü
	unsigned char bCmdType;
	unsigned char bCmdParam;		//AF?üá?ààDí
	unsigned int nIRCut;		//IRCut1|?ü
}T_LENS_IRCUT;

typedef struct {//0x34.éè???μí·IRCut1|?ü
	unsigned char bCmdType;
	unsigned char bCmdParam;		
	unsigned int nStep;		
}T_LENS_IRIS;

typedef struct {//0x34.éè???μí·IRCut1|?ü
	unsigned char bCmdType;
	unsigned char bCmdParam;		//AF?üá?ààDí
	unsigned int nStatus;		//
	///1-44  44对应18倍
	unsigned int nZoomSection;		//ZOOM??μ?????
	int nZoomPos;		//镜头位置20--2020
	int nFocusPos;		//聚焦位置
	unsigned int nCurTime;
	unsigned char bDayNight;		///白天 1  晚上0
	unsigned char bResetRouteFlag;	///用于控球导致的场景改变而重新高倍到低倍时定位曲线
	unsigned char LenRatio;
	Handle_get_ircut_staus_callback_t get_ircut_staus;
}T_LENS_STATUS;

typedef struct {//0x34.éè???μí·IRCut1|?ü
	unsigned char bCmdType;
	unsigned char bCmdParam;		
	unsigned int nRoute;		
}T_LENS_CONFIG;

typedef struct {//
	unsigned char bCmdType;
	unsigned char bCmdParam;		
	unsigned char nToDayValue;		
	unsigned char nToNightValue;		
}T_LENS_DAYNIGHT;

typedef struct {//0x34.éè???μí·IRCut1|?ü
	unsigned char bCmdType;
	unsigned char bCmdParam;		// 1,自动 0，手动		
}T_LENS_FOCUSMANAUTO;


typedef struct {//0x34.éè???μí·IRCut1|?ü
	unsigned char bCmdType;
	unsigned char bCmdParam;		// 1,启动控球聚焦 0，否		
}T_LENS_DOMECTRLAF;

typedef struct {//0x34.éè???μí·IRCut1|?ü
	unsigned char bCmdType;
	unsigned short Ratio;		// 倍率，visca协议转换后	
	int focuspos;				
}T_LENS_ZOOMRATIO;

typedef struct {//0x34.éè???μí·IRCut1|?ü
	unsigned char bCmdType;
	unsigned short Ratio;		// 倍率，visca协议转换后	
	int focuspos;				
}T_LENS_ZOOMDIRECT;

typedef struct {//0x34.éè???μí·IRCut1|?ü
	unsigned char bCmdType;
	unsigned short Ratio;		// 倍率，visca协议转换后	
	int focuspos;				
}T_LENS_FOCUSDIRECT;

typedef struct {//0x34.éè???μí·IRCut1|?ü
	unsigned char bCmdType;			
}T_LENS_RESET;
#pragma pack(0)

#endif

void *VIDEO_afMsgMain(void *arg);
void *Video_afMain(void *arg);


#if defined (__cplusplus)
}
#endif

#endif

