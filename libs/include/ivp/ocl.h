/*************************************************************************************//**
 *  @file       ocl.h
 *
 *  @brief      Brief description of ocl.h
 *
 *  @date       2016-11-30 10:32
 *         
 **************************************************************************************/


#ifndef _MML_OCL_H_
#define _MML_OCL_H_

#include <mml/core/core.h>

typedef void * (*OCLCB) (void * parg);

typedef enum _OCLAlarm{
    OCL_ALARM_CLEAR = 0,
    OCL_ALARM_OVERCROWD = 1,
} OCLAlarm;

typedef enum _OCLError{
    OCL_UNKNOWN = -1,
    OCL_OK = 0,
} OCLError;

typedef struct _OCLProp {
    MMLSize img_size;
} OCLProp;

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * oclInit Initialize OCL
 * @return ocl handle pointer
 */
void * oclInit(OCLProp prop);

/**
 * oclFinal Finalize OCL
 * @param h_ocl [in] ocl handle pointer pointer
 * @return exec status
 */
int oclFinal(void ** h_ocl);

/**
 * oclRegCB Regist OCL Callback Function
 * @param h_ocl [in] ocl handle pointer
 * @return exec status
 */
int oclRegCB(void * h_ocl, OCLCB cb);

/**
 * oclSetRegion Set OCL Region
 * @param h_ocl [in] ocl handle pointer
 * @param rect [in] Region rectangle
 * @return exec status
 */
int oclSetRegion(void * h_ocl, MMLRect rect);

/**
 * oclSetTargetNum Set OCL Region target number
 * @param h_ocl [in] ocl handle pointer
 * @param rect [in] Region rectangle
 * @return exec status
 */
int oclSetTargetNum(void * h_ocl, int number);

/**
 * oclSetTargetBias Set OCL Region target bias
 * @param h_ocl [in] ocl handle pointer
 * @param rect [in] Region rectangle
 * @return exec status
 */
int oclSetTargetBias(void * h_ocl, int number);

/**
 * oclIsDrawEnable Is OCL draw target Enable
 * @param h_ocl [in] ocl handle pointer
 * @return enable status
 */
bool oclIsDrawEnable(void * vh_ocl);

/**
 * oclDrawEnable Enable OCL draw target
 * @param h_ocl [in] ocl handle pointer
 * @return exec status
 */
int oclDrawEnable(void * h_ocl);

/**
 * oclDrawEnable Enable OCL draw target
 * @param h_ocl [in] ocl handle pointer
 * @return exec status
 */
int oclDrawDisable(void * h_ocl);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* !_MML_OCL_H_ */

