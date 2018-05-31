#ifndef __PTZIMPL_H__
#define __PTZIMPL_H__

#include <termios.h>

#define BYTE unsigned char
#define BOOL int
#define FALSE 0
#define TRUE 1


/*
typedef struct tagPORTPARAMS
{
	int		nBaudRate;      // BAUDRATE_ENUM
	int		nCharSize;      // CHARSIZE_ENUM
	int		nParityBit;     // PARITYBIT_ENUM
	int		nStopBit;       // STOPBIT_ENUM
	int		fCRTSCTS;       // Hardware Flow Control(1/0)
	int		fXONXOFF;       // Sofwware Flow Control(1/0)
} NC_PORTPARAMS, *PNC_PORTPARAMS;
*/


////////////////////////////////////////////////////////////////////////////////

enum _PTZ_COMMAND_
{
	PANTILT_NONE,					// None
	PANTILT_PAN_LEFT,				// PanLeft
	PANTILT_PAN_RIGHT,				// PanRight
	PANTILT_TILT_UP,				// TiltUp
	PANTILT_TILT_DOWN,				// TiltDown
	PANTILT_RESET,					// Reset
	PANTILT_ZOOM_IN,				// ZoomIn
	PANTILT_ZOOM_OUT,				// ZoomOut
	PANTILT_FOCUS_NEAR,				// FocusNear
	PANTILT_FOCUS_FAR,				// FocusFar
	PANTILT_IRIS_OPEN,				// IrisOpen
	PANTILT_IRIS_CLOSE,				// IrisClose
	PANTILT_COUNT					// PanTilt Count
};

enum _PTZ_DEVICE_ID_
{
	PANTILTDEVICE_PELCO_P,			// PELCO-P
	PANTILTDEVICE_PELCO_D,			// PELCO-D

	PANTILTDEVICE_MAXID
};

////////////////////////////////////////////////////////////////////////////////

typedef struct tagCOMMPELCOP
{
	BYTE		Stx;				// Stx
	BYTE		Addr;				// Address
	BYTE		Data1;				// Data1
	BYTE		Data2;				// Data2
	BYTE		Data3;				// Data3
	BYTE		Data4;				// Data4
	BYTE		Etx;				// Etx
	BYTE		CheckSum;			// CheckSum
} COMMPELCOP, *PCOMMPELCOP;

typedef struct tagCOMMPELCOD
{
	BYTE		Synch;				// Stx
	BYTE		Addr;				// Address
	BYTE		Cmd1;				// Command 1
	BYTE		Cmd2;				// Command 2
	BYTE		Data1;				// Data 1
	BYTE		Data2;				// Data 2
	BYTE		CheckSum;			// CheckSum
} COMMPELCOD, *PCOMMPELCOD;

////////////////////////////////////////////////////////////////////////////////

//extern int  GetFD485(void);
//extern int  Init485();
//extern int  InitPTZPort(int fd, const NC_PORTPARAMS *param);
//extern int  Open485(void);
//extern void Deinit485(void);
//extern BOOL WriteTo485(void *pdata, int nsize);

//extern BOOL ControlPELCO_D(int nAddr, int nControl);

#endif//	__PTZIMPL_H__

