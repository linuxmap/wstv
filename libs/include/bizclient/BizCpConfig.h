/******************************************************************************

  Copyright (c) 2013 . All rights reserved.

  ******************************************************************************
  File Name     : BizCpConfig.h
  Version       : Daomeng Han <itangwang@126.com>
  Author        : Daomeng Han <itangwang@126.com>
  Created       : 2015-01-07
  Description   : Biz cross platform library
  History       : 
  1.Date        : 2015-01-07
    Author      : Daomeng Han <itangwang@126.com>
	Modification:
******************************************************************************/

#ifndef _BIZ_CP_CONFIG_H_
#define _BIZ_CP_CONFIG_H_


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

	#ifdef WIN32
		#ifdef BIZLIB_EXPORTS
			#define BIZLIB_API __declspec(dllexport)
		#else
			#define BIZLIB_API __declspec(dllimport)
		#endif
		#include <BizCpWin32Config.h>
	#else /* LINUX */
		#define BIZLIB_API
		#include <BizCpLinuxConfig.h>
	#endif

	#define BIZCP_E_OK         0
	#define BIZCP_E_ERROR      -1
	#define BIZCP_E_AGAIN      -2
	#define BIZCP_E_BUSY       -3
	#define BIZCP_E_DONE       -4
	#define BIZCP_E_DECLINED   -5
	#define BIZCP_E_ABORT      -6
	#define BIZCP_E_AUTH       -7
	#define BIZCP_E_HOST       -8	/* Dns Host error */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef _BIZ_CP_CONFIG_H_ */
