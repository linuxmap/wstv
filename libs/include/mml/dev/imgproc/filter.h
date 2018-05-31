/************************************************************************************//**
 *  @file       filter.c
 *
 *  @brief      Image Filter 
 *
 *  @date       2016-08-05 10:15
 *
 ***************************************************************************************/

#ifndef _MML_DEV_FILTER_H_
#define _MML_DEV_FILTER_H_

#include "mml/core/core.h"
#include "mml/imgproc/imgproc_c.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlDevGaussianBlur Gaussian Blur
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param size [in] Gaussian kernel size
 * @return 0 And status
 */
int mmlDevGaussianBlur(const MMLImage * src, MMLImage * dst, int size);

/**
 * mmlDevFakeAvgBlur Fake Average Blur
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param size [in] Filter kernel size
 * @return 0 And status
 */
int mmlDevFakeAvgBlur(const MMLImage * src, MMLImage * dst, int size);

/**
 * mmlDevMedianBlur Median Blur
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param size [in] Median kernel size
 * @return 0 And status
 */
int mmlDevMedianBlur(const MMLImage * src, MMLImage * dst, int size);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* !_MML_DEV_FILTER_H_ */
