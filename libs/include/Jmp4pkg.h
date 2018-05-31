#ifndef _JVS_PKG_H
#define _JVS_PKG_H

/*****************************************************************************
Copyright (C), 2012-2015, Jovision Tech. co., Ltd.
File Name:      Jmp4pkg.h
Author:         wxy(walker)
Description:
1. write a frame of h264/h.265/samr/alaw/ulaw data to a mp4 file
2. read a frame of h264/h.265/samr/alaw/ulaw data from a mp4 file
3. pkg with upk
4. with pts
*****************************************************************************/

#ifdef WIN32

typedef __int64             int64_t;
typedef unsigned __int64    uint64_t;

#ifdef __cplusplus
#define JVS_API extern "C" __declspec(dllexport)
#else
#define JVS_API __declspec(dllexport)
#endif

#else // not windows

typedef long long           int64_t;
typedef unsigned long long  uint64_t;	

#ifdef __cplusplus
#define JVS_API extern "C"
#else
#define JVS_API
#endif

#endif

/* 2015-10-23 JP_UnpkgOneFrame*/
#define MP4_RET_OK			1
#define MP4_RET_FAIL		0
#define MP4_RET_INPUT_ERR	-9
#define MP4_RET_WRITE_FAIL  -10
#define MP4_RET_READ_FAIL   -11
#define MP4_RET_FRAME_ERR   -12
#define MP4_RET_FRAMENO_ERR -13

/*
 *pkg
 */

/*
 *JP_OpenPackage param
 */

// video type default h264
#define JVS_VCODEC_H264 0x100
#define JVS_VCODEC_HEVC 0x200

// audio type
#define JVS_ACODEC_SAMR     0
#define JVS_ACODEC_ALAW     1
#define JVS_ACODEC_ULAW     2

// mp4 sps\pps 
// sps\pps must del start code 0 0 0 1
// if you do not use, iSpsSize = 0; pPps = NULL
typedef struct _MP4_AVCC
{
    int                 iSpsSize;
    int                 iPpsSize;
    unsigned char *     pSps;
    unsigned char *     pPps;
}MP4_AVCC;

// video type
typedef struct _PKG_VIDEO_PARAM				
{
    int                 iFrameWidth;
    int                 iFrameHeight;
    float               fFrameRate;
    MP4_AVCC            avcC;																			
}PKG_VIDEO_PARAM, * PPKG_VIDEO_PARAM;

/*
 *JP_PackageOneFrame param
 */
// avPkt.type
#define JVS_PKG_VIDEO 1
#define JVS_PKG_AUDIO 2

// input
typedef struct _AV_PACKET
{
    unsigned int        iType;  // frame type (video/audio) 
    unsigned char *     pData;	// frame data pointer
    unsigned int        iSize;  // frame size
    int64_t             iPts;	// In milliseconds, incoming time counts
    int64_t             iDts;   // no b frame iDts = 0
}AV_PACKET, * PAV_PACKET;


#ifndef	WIN32
/*****************************************************************************
 *JP_InitSDK:
 *1. each channel allocate iSize bytes frame buffer
 *2. the first call and called only once (pkg)
 *3. If non windows, first wrote the frame buffer, and then write the file
 *In:     int iSize  // each channel allocate iSize bytes frame buffer
 *In:     int	iCount // after V1.2.1.7 2015-06-10 not used
 *Return: return 1, if successful, or 0 if an error occurred 
******************************************************************************/ 
JVS_API	int JP_InitSDK(int iSize, int iCount);

/*****************************************************************************
 *JP_ReleaseSDK:
 *release the allocated buffer (pkg)
 *Return:	  
*****************************************************************************/ 
JVS_API	void JP_ReleaseSDK();
#endif

typedef void * MP4_PKG_HANDLE;  //pkg handle

/*****************************************************************************
 *JP_OpenPackage:
 *Create a pkg instance
 *In:     PPKG_VIDEO_PARAM pVideoParam
 *In:     int              bWriteVideo
 *In:     int              bWriteAudio
 *In:	  char *           pszmp4file
 *In:     char *           pszjdxFile // 1. if not close mp4file, jdx file is
 *                                    // used to assist read mp4 file. 
 *                                    // 2. alos can be NULL
 *In:     int              iAVCodec   // pkg audio/video type
 *                         iACodec = JVS_ACODEC_ULAW;
 *                         V1.2.1.0 2015-03-19
 *                         iAVCodec = JVS_VCODEC_HEVC or JVS_VCODEC_H264;
 * 						   iAVCodec = 0; default JVS_VCODEC_H264
 *                         iAVCodec |= iACodec;
 *In:     int              iChannel   // channel nubmer(windows 0)										
 *Return: return a handle to the newly-created instance,
 *        or NULL if an error occurred  
*****************************************************************************/ 
JVS_API	MP4_PKG_HANDLE JP_OpenPackage(PPKG_VIDEO_PARAM pVideoParam,
                                      int              bWriteVideo, 
                                      int              bWriteAudio, 
                                      char *           pszmp4file,
                                      char *           pszIdxFile,
                                      int              iAVCodec,
                                      int              iChannel);

/*****************************************************************************
 *JP_ClosePackage:
 *destroy a pkg instance
 *In:     MP4_PKG_HANDLE	h
 *Return: 
*****************************************************************************/ 
JVS_API	void JP_ClosePackage(MP4_PKG_HANDLE h);

/*****************************************************************************
 *JP_PackageOneFrame:
 *pkg a h264/samr/alaw/ulaw frame
 *In:     MP4_PKG_HANDLE h
 *In:     PAV_PACKET     pAVPkt // h264/h.265/audio input struct
 *Return: return 1 if successful, or <= 0 if an error occurred	  
*****************************************************************************/ 
JVS_API int JP_PackageOneFrame(MP4_PKG_HANDLE h, PAV_PACKET pAVPkt);


/*
 * upk
 */

// JP_OpenUnpkg, return mp4 info, this struct hold by caller
typedef struct _MP4_INFO
{
    char            szVideoMediaDataName[8]; 
	                                   // video encoder name "avc1" or "hev1"
    unsigned int    iFrameWidth;             // video width
    unsigned int    iFrameHeight;            // video height
    unsigned int    iNumVideoSamples;        // video num
    double          dFrameRate;              // video rate
    char            szAudioMediaDataName[8]; // "samr" "alaw" "ulaw"
    unsigned int    iNumAudioSamples;        // audio sample num
}MP4_INFO, *PMP4_INFO;


// JP_UnpkgOneFrame input avUnpkt.type
#define JVS_UPKT_VIDEO 1     // get one frame video
#define JVS_UPKT_AUDIO 2     // get one frame audio

// JP_UnpkgOneFrame input/out, this struct hold by caller
typedef struct _AV_UNPKT
{
    // in:  upk type (video or audio)
    unsigned int	iType;
    // in:  sample id (1 -- iNumVideoSamples/iNumAudioSamples)
    unsigned int    iSampleId;
    // out: upk output h264/h265/samr/alaw/ulaw data, hold byte upk internal
    unsigned char * pData;
    // out: upk output h264/h265/samr/alaw/ulaw data size
    unsigned int    iSize;
    // out: upk output pts, ms time count
    uint64_t        iSampleTime;
    // out: upk output is h264 idr frame
    int             bKeyFrame;
}AV_UNPKT, *PAV_UNPKT;

typedef void * MP4_UPK_HANDLE; // upk handle

/*****************************************************************************
 *JP_OpenUnpkg:
 *Create a upk instance
 *In:     char *       pszmp4file
 *In:     PMP4_INFO    pMp4Info
 *In:     unsigned int iBufSize // setvbuf size linux dvr/nvr used if 0, 64kB
 *Return: return a handle to the newly-created instance,
 *        or NULL if an error occurred 
*****************************************************************************/ 
JVS_API MP4_UPK_HANDLE JP_OpenUnpkg(char *pszmp4file,
                                   PMP4_INFO pMp4Info, unsigned int iBufSize);


/*****************************************************************************
 *JP_CloseUnpkg:
 *destroy a upk instance
 *In:     MP4_PKG_HANDL h
 *Return:  
*****************************************************************************/
JVS_API void JP_CloseUnpkg(MP4_UPK_HANDLE h);



/*****************************************************************************
 *JP_UnpkgOneFrame:
 *upk a h264/samr/alaw/ulaw frame
 *In:     MP4_UPK_HANDLE h
 *In:     PAV_UNPKT      pAvUnpkt
 *Return: return 1 if successful, or <= 0 if an error occurred  
*****************************************************************************/ 
JVS_API int JP_UnpkgOneFrame(MP4_UPK_HANDLE h, PAV_UNPKT pAvUnpkt);


/*****************************************************************************
 *JP_UnpkgKeyFrame:
 *find the key frame of near the frameno
 *In:     MP4_UPK_HANDLE h
 *In:     unsigned int    iVFrameNo // > 0, if 0, return first key frame,
 *                                  // bForward invaild
 *In:     int            bForward  // Search direction
 *                                 // 1 forward(to right), 0 backward(to left) 
 *Return: the key frmae no if successful, -2 if an error occurred, 
 *        -1, not found
*****************************************************************************/ 
JVS_API int JP_UnpkgKeyFrame(MP4_UPK_HANDLE h,
                                        unsigned int iVFrameNo, int bForward);

/*****************************************************************************
 *JP_PkgGetAudioSampleId:
 *get an audio frame number,
 *the PTS of this audio frame is no more than a video PTS
 *In:     MP4_UPK_HANDLE h
 *In:     unsigned int   iVFrameNo // > 0
 *In/Out: uint64_t * piVPts        // return video pts, can be input NULL
 *In/Out: uint64_t * piAPts        // return audio pts, can be input NULL
 *Return: an audio frame number if successful, -2 if an error occurred, 
 *        -1, not found
*****************************************************************************/ 
JVS_API int JP_PkgGetAudioSampleId(MP4_UPK_HANDLE h, unsigned int iVFrameNo, 
                                       uint64_t * piVPts, uint64_t * piAPts);

/*
 *if mp4 not close, used read mp4file
 */
typedef void *	MP4_FILE_HANDLE; // no normal mp4 file handle

//JP_CheckFile****** V1.2.1.0 2015-03-19 also can be not used!!!!! *****
// JP_CheckFile output, hold by caller
typedef struct _MP4_CHECK
{
    int             bNormal;    // output: 1, normal file, 0, not close file
    unsigned int    iDataStart;	// output: not used!
}MP4_CHECK, *PMP4_CHECK;

/*****************************************************************************
 *JP_CheckFile: 
 *note: V1.2.1.0 2015-03-19 also can be not used!!! width JP_OpenUnpkg replace
 *check if the file has been closed, and then decide how upk mp4 file.
 *if pMp4Check.bNormal =1, call JP_OpenUnpkg, else call JP_OpenFile
 *if upk mp4 file, please first call
 *In:     char *        pszmp4file
 *In:     PMP4_CHECK    pMp4Check
 *Return: return 1 if successful, or 0 if an error occurred  	  
******************************************************************************/ 
JVS_API int JP_CheckFile(char * pszmp4file, PMP4_CHECK pMp4Check);

// JP_OpenFile output, hold by caller
typedef struct _JP_MP4_INFO
{
    int             bHasVideo;
    int             bHasAudio;
    char            szVideoMediaDataName[8];
    unsigned int    iFrameWidth;
    unsigned int    iFrameHeight;
    double          dFrameRate;
    unsigned int    iNumVideoSamples;
    char            szAudioMediaDataName[8];
    unsigned int    iNumAudioSamples;
}JP_MP4_INFO, *PJP_MP4_INFO;
/*****************************************************************************
 *JP_OpenFile:
 *Create a upk instance
 *In:     char *        pszmp4file
 *In:     unsigned int  iDataStart // pMp4Check.iDataStart
 *                                 // V1.2.1.0 2015-03-19 not used 
 *In:     char *        pszIdxFile // need jdx,must input
 *In:	  unsigned int 	iBufSize   // setvbuf size linux dvr used if 0, 64kB
 *Return: return a handle to the newly-created instance,
 *        or NULL if an error occurred 	  
*****************************************************************************/
JVS_API MP4_FILE_HANDLE	JP_OpenFile(char *	        pszmp4file,
                                    unsigned int    iDataStart,
                                    char *          pszIdxFile,
                                    PJP_MP4_INFO	pMp4Info,
                                    unsigned int	iBufSize);

/*****************************************************************************
 *JP_ReadFile:
 *upk a h264/h265/samr/alaw/ulaw frame
 *In:     MP4_FILE_HANDLE h
 *In:     PAV_UNPKT       pAvUnpkt
 *Return: return 1 if successful, or 0 no data   
*****************************************************************************/ 
JVS_API int JP_ReadFile(MP4_FILE_HANDLE h, PAV_UNPKT pAvUnpkt);


/*****************************************************************************
 *JP_CloseFile:
 *destroy a upk instance
 *In:     MP4_FILE_HANDLE h
 *Return: 
*****************************************************************************/
JVS_API void JP_CloseFile(MP4_FILE_HANDLE h);


/*****************************************************************************
 *JP_ReadKeyFrame:
 *find the key frame of near the frameno
 *In:     MP4_FILE_HANDLE h
 *In:     unsigned int    iVFrameNo // > 0, if 0, return first key frame,
 *                                  // bForward invaild
 *In:     int             bForward  // Search direction
 *                                  // 1 forward(to right), 0 backward(to left) 
 *Return: the key frmae no if successful, -2 if an error occurred, 
 *        -1, not found
*****************************************************************************/ 
JVS_API int JP_ReadKeyFrame(MP4_FILE_HANDLE h, 
                                        unsigned int iVFrameNo, int bForward);

/*****************************************************************************
 *JP_JdxGetAudioSampleId:
 *get an audio frame number,
 *the PTS of this audio frame is no more than a video PTS
 *In:     MP4_FILE_HANDLE h
 *In:     unsigned int   iVFrameNo // > 0
 *In/Out: uint64_t * piVPts        // return video pts, can be input NULL
 *In/Out: uint64_t * piAPts        // return audio pts, can be input NULL
 *Return: an audio frame number if successful, -2 if an error occurred, 
 *        -1, not found
*****************************************************************************/ 
JVS_API int JP_JdxGetAudioSampleId(MP4_FILE_HANDLE h, unsigned int iVFrameNo, 
                                       uint64_t * piVPts, uint64_t * piAPts);

#endif	// _JVS_PKG_H
