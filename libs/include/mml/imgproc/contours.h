/******************************************************************************
 *  @file       contours.h
 *
 *  @brief      Brief description of contours.h
 *
 *  @date       2016-06-01 15:31
 *  @author     liuchen - liuchen@jovision.com
 *
 *****************************************************************************/

#ifndef _MML_CONTOURS_H_
#define _MML_CONTOURS_H_

#include "mml/core/core.h"

typedef enum _MMLCCLType {
    MML_CCL_RECT = 0,
    MML_CCL_DETAIL,
    MML_CCL_CONTOUR,
    MML_CCL_BUTT,
} MMLCCLType;

typedef struct _MMLCCLHandle {
    MMLCCLType ccl_type;
    MMLMemStorage * mem;
} MMLCCLHandle;

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlCCLInit Init CCL module
 * @return h mml CCL handle
 */
MMLCCLHandle * mmlCCLInit(MMLCCLType ccl_type);

/**
 * mmlCCLInit Init CCL module
 * @param h_ccl [in] ccl handle pointer
 * @param img [in] image need be labeled
 * @return MMLError final status
 */
int mmlCCL(MMLCCLHandle * h_ccl, const MMLImage * img);

/**
 * mmlCCLInit Init CCL module
 * @param h_ccl [in] mml ccl handle pointer pointer
 * @return MMLError final status
 */
int mmlCCLFinal(MMLCCLHandle ** h_ccl);

/**
 * mmlCCLBoundingRect Get ccl Bounding Rect
 * @param h_ccl [in] ccl handle pointer
 * @param sn [in] ccl serial number
 * @param ccl_rect [out] ccl bounding rect
 * @return MMLError final status
 */
int mmlCCLBoundingRect(MMLCCLHandle * h_ccl, int sn, MMLRect * ccl_rect);

/**
 * mmlCCLArea Get cc area
 * @param h_ccl [in] ccl handle pointer
 * @param sn [in] ccl serial number
 * @return area cc area
 */
int mmlCCLArea(MMLCCLHandle * h_ccl, int sn);

/**
 * mmlCCLNum Get ccl region number
 * @param h_ccl [in] ccl handle pointer
 * @return num ccl number
 */
int mmlCCLNum(MMLCCLHandle * h_ccl);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* !_MML_CONTOURS_H_ */

