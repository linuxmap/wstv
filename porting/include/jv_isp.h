#ifndef _JV_ISP_H_
#define _JV_ISP_H_

#include"jv_common.h"
#include <utl_filecfg.h>

int  jv_isp_get_sensor(int *value);

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
/*
//gyd add for  isp control   20130415
*/
#define WEIGHT_ZONE_ROW			15
#define WEIGHT_ZONE_COLUMN		17


typedef enum 
{
	JV_OP_TYPE_AUTO	= 0,
	JV_OP_TYPE_MANUAL	= 1,
	JV_OP_TYPE_BUTT
    
} JV_ISP_OP_TYPE_E;

typedef enum {
    JV_HI_FALSE = 0,
    JV_HI_TRUE  = 1,
} JV_HI_BOOL;

typedef enum 
{
    JV_AE_MODE_LOW_NOISE		= 0,
    JV_AE_MODE_FRAME_RATE		= 1,
    JV_AE_MODE_BUTT
    
} JV_ISP_AE_MODE_E;
typedef enum
{
        JV_ISP_AE_FRAME_END_UPDATE_0  = 0x0, //isp update gain and exposure  in the  same frame
        JV_ISP_AE_FRAME_END_UPDATE_1  = 0x1, //isp update exposure one frame before  gain
       
        JV_ISP_AE_FRAME_END_BUTT

}JV_ISP_AE_FRAME_END_UPDATE_E;


typedef struct 
{
	JV_HI_BOOL bEnable;
    
} JV_ISP_GAMMA_ATTR_S;

typedef enum
{
	JV_ISP_GAMMA_CURVE_1_6 = 0x0,           /* 1.6 Gamma curve */
	JV_ISP_GAMMA_CURVE_1_8 = 0x1,           /* 1.8 Gamma curve */
	JV_ISP_GAMMA_CURVE_2_0 = 0x2,           /* 2.0 Gamma curve */
	JV_ISP_GAMMA_CURVE_2_2 = 0x3,           /* 2.2 Gamma curve */
	JV_ISP_GAMMA_CURVE_DEFAULT = 0x4,       /* default Gamma curve */
	JV_ISP_GAMMA_CURVE_SRGB = 0x5,
	JV_ISP_GAMMA_CURVE_USER_DEFINE = 0x6,   /* user defined Gamma curve, Gamma Table must be correct */
	JV_ISP_GAMMA_CURVE_BUTT
	
} JV_ISP_GAMMA_CURVE_E;


/*
AE ctrl
*/


typedef struct 
{
//曝光类型
    JV_ISP_OP_TYPE_E type;

//手动曝光属性
    int s32AGain; 
    int s32DGain; 
    unsigned int u32ExpLine; 
    JV_HI_BOOL bManualExpLineEnable; 
    JV_HI_BOOL bManualAGainEnable; 
    JV_HI_BOOL bManualDGainEnable; 

//自动曝光属性
    JV_ISP_AE_MODE_E enAEMode;		
    unsigned short u16ExpTimeMax;       
    unsigned short u16ExpTimeMin;       
    unsigned short u16DGainMax;        
    unsigned short u16DGainMin;         
    unsigned short u16AGainMax;			
    unsigned short u16AGainMin;         
    unsigned char  u8ExpStep;			
    short s16ExpTolerance;		
    unsigned char  u8ExpCompensation;	
    JV_ISP_AE_FRAME_END_UPDATE_E  enFrameEndUpdateMode;
    JV_HI_BOOL bByPassAE;
    union{
		unsigned char u8Weight[WEIGHT_ZONE_ROW][WEIGHT_ZONE_COLUMN];
		unsigned char u8WeightLine[WEIGHT_ZONE_ROW*WEIGHT_ZONE_COLUMN];
	};
    
    
}jvExposure_t;

 

int jv_isp_exposure_set(jvExposure_t *exposure);

int jv_isp_exposure_get(jvExposure_t *exposure);

/*
WB ctrl
*/
typedef struct
{
      //wb mode
      JV_ISP_OP_TYPE_E type;

      //manual mode
      unsigned short u16Rgain;      
      unsigned short u16Ggain;      
      unsigned short u16Bgain;

    //auto mode
    unsigned char u8RGStrength;        
    unsigned char u8BGStrength;        
    unsigned char u8ZoneSel;          
    unsigned char u8HighColorTemp;     
    unsigned char u8LowColorTemp;     
    //unsigned char u8Weight[WEIGHT_ZONE_ROW][WEIGHT_ZONE_COLUMN]; 
    union{
		unsigned char u8Weight[WEIGHT_ZONE_ROW][WEIGHT_ZONE_COLUMN];
		unsigned char u8WeightLine[WEIGHT_ZONE_ROW*WEIGHT_ZONE_COLUMN];
	};
    
    
}jvWb_t;



int jv_isp_wb_set(jvWb_t *wb);

int jv_isp_wb_get(jvWb_t *wb);


/*
gamma ctrl
*/
typedef struct
{
    JV_ISP_GAMMA_ATTR_S bEnable; 
    JV_ISP_GAMMA_CURVE_E enGammaCurve; 
    unsigned short u16Gamma[257]; 
    unsigned short u16Gamma_FE[257];


    
    
}jvGamma_t;


int jv_isp_gamma_set(jvGamma_t *gamma);

int jv_isp_gamma_get(jvGamma_t *gamma);


/*
sharpen ctrl
*/
typedef struct 
{
    JV_HI_BOOL bEnable;
	JV_HI_BOOL bManualEnable;
	unsigned char u8StrengthTarget;   
	unsigned char u8StrengthMin;      
    unsigned char u8SharpenAltD[8]; 
    unsigned char u8SharpenAltUd[8]; 

   
}jvSharpen_t;


int jv_isp_sharpen_set(jvSharpen_t *sharpen);

int jv_isp_sharpen_get(jvSharpen_t *sharpen);


/*CCM ctrl*/

typedef struct 
{
    JV_HI_BOOL bSatManualEnable; 
    unsigned char u8SatTarget; 
    unsigned char au8Sat[8]; 

    unsigned int pu32Value;
    
    unsigned short u16HighColorTemp; 
    unsigned short au16HighCCM[9]; 
    unsigned short u16MidColorTemp; 
    unsigned short au16MidCCM[9]; 
    unsigned short u16LowColorTemp; 
    unsigned short au16LowCCM[9]; 

    

    
}jvCcm_t;

int jv_isp_ccm_set(jvCcm_t * ccm);

int jv_isp_ccm_get(jvCcm_t * ccm);


typedef struct
{
	jvExposure_t exposure;
	jvWb_t	     wb;
	jvGamma_t    gamma;
	jvSharpen_t	 sharpen;
	jvCcm_t      ccm;

}jvISP_t;


int jv_isp_cfg_set(char *key, int value);

char *jv_isp_cfg_get(char *key);
	
int jv_isp_cfg_flush();

void jv_isp_cfg_init();
#endif

