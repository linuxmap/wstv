/******************************************************************************
 *  @file       contours.h
 *
 *  @brief      Brief description of contours.h
 *
 *  @date       2016-06-01 15:31
 *  @author     liuchen - liuchen@jovision.com
 *
 *****************************************************************************/

#ifndef _MML_DEV_CONTOURS_H_
#define _MML_DEV_CONTOURS_H_

#include "mml/core/core.h"
#include "mml/imgproc/imgproc_c.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlCCLInit Init CCL module
 * @return h mml CCL handle
 */
MMLCCLHandle * mmlDevCCLInit(MMLCCLType ccl_type);

/**
 * mmlDevCCLInit Init CCL module
 * @param h_ccl [in] mml ccl handle pointer
 * @param img [in] image need be labeled
 * @return MMLError final status
 */
int mmlDevCCL(MMLCCLHandle * h_ccl, const MMLImage * img);

/**
 * mmlCCLInit Init CCL module
 * @param h_ccl [in] mml ccl handle pointer pointer
 * @return MMLError final status
 */
int mmlDevCCLFinal(MMLCCLHandle ** h_ccl);

/**
 * mmlDevCCLBoundingRect Get ccl Bounding Rect
 * @param h_ccl [in] ccl handle pointer
 * @param sn [in] ccl serial number
 * @param ccl_rect [out] ccl bounding rect
 * @return MMLError final status
 */
int mmlDevCCLBoundingRect(MMLCCLHandle * h_ccl, int sn, MMLRect * ccl_rect);

/**
 * mmlDevCCLArea Get cc area
 * @param h_ccl [in] ccl handle pointer
 * @param sn [in] ccl serial number
 * @return area cc area
 */
int mmlDevCCLArea(MMLCCLHandle * h_ccl, int sn);

/**
 * mmlDevCCLNum Get ccl region number
 * @param h_ccl [in] ccl handle pointer
 * @return num ccl number
 */
int mmlDevCCLNum(MMLCCLHandle * h_ccl);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* !_MML_CONTOURS_H_ */

