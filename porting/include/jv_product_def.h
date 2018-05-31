#ifndef _JV_PRODUCT_DEF_H_
#define _JV_PRODUCT_DEF_H_


// 硬件型号定义，默认为海思方案
typedef enum
{
	HW_TYPE_UNKNOWN,
	HW_TYPE_HA210,
	HW_TYPE_HA230,
	HW_TYPE_A4,		// 18EV200 新A4
	HW_TYPE_C3,		// 注意，此C3指海思平台C3H(HC301)或C3A，非国科C3A(HC300)
	HW_TYPE_C5,
	HW_TYPE_C8A,
	HW_TYPE_C8,
	HW_TYPE_C9,
	HW_TYPE_V3,
	HW_TYPE_V6,
	HW_TYPE_V7,
	HW_TYPE_V8,		// V8和C8有所差异
	HW_TYPE_C8S,	//16EV100平台
	HW_TYPE_C8H,	//16EV100平台
	HW_TYPE_C3W,	//16EV100平台
}HW_TYPE_e;

// 检查硬件型号是否匹配
#define HWTYPE_MATCH(type)		((type) == hwinfo.HwType)
#define HWTYPE_NOT_MATCH(type)	(!HWTYPE_MATCH(type))

// 检查当前设备型号是否为model
#define PRODUCT_MATCH(model)		(0 == strcmp(hwinfo.devName, (model)))
#define PRODUCT_NOT_MATCH(model)	(!PRODUCT_MATCH(model))


// 卡片机
#define PRODUCT_HA210			"HA210"			// 100W卡片机
#define PRODUCT_HA230			"HA230"			// 200W卡片机
#define PRODUCT_HA410			"HA410"			// 新A4，100W卡片机（暂无正式产品）

// 摇头机
#define PRODUCT_C3A				"HC300"			// 100W摇头机（国科方案）
#define PRODUCT_C3H				"HC301"			// 200W摇头机
#define PRODUCT_C4L				"C4L"			// 200W摇头机（基于C3H硬件，流媒体协议）
#define PRODUCT_C5				"HC530"			// 100W摇头机
#define PRODUCT_C5S				"HC531"			// 200W摇头机
#define PRODUCT_C6S				"C6S"			// 200W摇头机,C5S基础上添加智能化

#define PRODUCT_C5S_M			"C5S-M"			// 200W摇头机（澜起6700WiFi）
#define PRODUCT_V3				"HV310"			// 130W摇头机
#define PRODUCT_V3H				"HV311"			// 200W摇头机(8188Wifi)
#define PRODUCT_V6				"HV610"			// 200W摇头机
#define PRODUCT_C3W				"C3W"			// 200W摇头机

#define PRODUCT_C3C				"HC310"			// 130W摇头机(海思方案，100W放大)
#define PRODUCT_C5_R1			"HC530-R1"		// 130W摇头机(100W放大)

// 摄录一体机
#define PRODUCT_V8A				"HV810A"		// 基础版V8，有音频不防水
#define	PRODUCT_V8				"HV810"			// V8升级版双天线，无喇叭，有MIC，WiFi8192
#define PRODUCT_C8A				"HC810A"		// 基础版C8，无音频防水
#define PRODUCT_C8				"HC810"			// C8升级版双天线，无音频，WiFi8192
#define PRODUCT_C8S				"C8S"
#define PRODUCT_V8S				"V8S"
#define PRODUCT_C8H				"C8H"

// 其他
#define PRODUCT_V7				"HV710"			// V7鱼眼
#define PRODUCT_C9				"HC910"			// C9智慧球

// 外贸定制
#define PRODUCT_HA230E			"HA230E"		// HA230外贸版，小维国际版app
#define PRODUCT_HA230C			"HA230C"		// HA230外贸版，CloudSee
#define PRODUCT_C3E				"HC310E"		// V3外贸版，小维国际版app
#define PRODUCT_C3HE			"HC301E"		// C3H中维外贸版，小维国际版app
#define PRODUCT_C8E				"HC810E"		// C8A中维外贸版，小维国际版app，7601 WiFi模块
#define PRODUCT_C8A_EN			"HC810A-EN"		// C8A小维外贸版，小维国际版app, 8188 WiFi模块

// 客户定制
#define PRODUCT_PBS				"PBS3801"		// 鹏博士定制产品，接入迅卫士平台，硬件与V8A一致
#define PRODUCT_ZHDZ			"SH-SX1"		// 中航电子定制产品，基于A4硬件
#define PRODUCT_AB1				"AI-AB1001"		// 爱维定制产品，基于新A4硬件
#define PRODUCT_C3H_AP			"HC301-AP"		// 客户定制C3HAP定制版本，使用8188WiFi
#define PRODUCT_HA230_AP		"HA230-AP"		// 客户定制HA230-AP定制版本，使用8188WiFi
#define PRODUCT_DQP_YTJ			"DQP-YTJ01"		// 迪奇普定制摇头机（6700Wifi），GPIO与C3H定义一致

#endif

