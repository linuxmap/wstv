/***********************************************************
*defs.h - defines
*
* Copyright(c) 2007~  
*
*$Date: $ 
*$Revision: $
*
*-----------------------
*$Log: $
*
*
*01a, 07-05-03, Zhushuchao created
*
************************************************************/
#ifndef __MAIN_DEFS_H_
#define __MAIN_DEFS_H_
#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32)
#define PACKED
#else
#define PACKED __attribute__((packed, aligned(1)))
#endif

#ifdef WIN32
typedef __int64  _INT64;
typedef unsigned __int64 _UINT64;
#else
typedef long long  _INT64;
typedef unsigned long long _UINT64;
#endif

typedef long _LONG;
typedef unsigned long _ULONG;

typedef int _INT32;
typedef unsigned int _UINT32;

typedef short _INT16;
typedef unsigned short _UINT16;

typedef signed char _INT8;
typedef unsigned char _UINT8;

typedef unsigned int _UINT;

typedef int _BOOL;

typedef volatile unsigned int _VUINT32;
typedef volatile unsigned short _VUINT16;
typedef volatile unsigned char _VUINT8;

#ifndef WIN32
typedef unsigned int HWND;
typedef unsigned int    WPARAM;
typedef unsigned long   LPARAM;
#endif

#ifndef MAX
#define MAX(x, y)           (((x) > (y))?(x):(y))
#endif

#ifndef MIN
#define MIN(x, y)           (((x) < (y))?(x):(y))
#endif

/*#ifndef ABS
#define ABS(x)              (((x)<0) ? -(x) : (x))
#endif*/

#define KB (1<<10)
#define MB (1<<20)
#define GB (1<<30)

#define BIT_SET(st, bit)  ((st) = (st)|(bit))
#define BIT_CLR(st, bit) ((st) = (st)&~(bit))
#define HAS_BIT(st, bit) (((st)&(bit)) == (bit))

typedef union
{
    _UINT8 b[4];
    _LONG l;
}UNION32;

typedef struct
{
    double x;
    double y;
}C_POINT_S;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef OK
#undef OK
#endif
#define OK 0

#ifdef ERROR
#undef ERROR
#endif
#define ERROR -1

#ifndef NOTSUPPORT
#define NOTSUPPORT -2
#endif

#ifdef WIN32
#ifdef DLL_EXPORTS
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif
#endif

#ifdef __cplusplus
}
#endif
#endif

