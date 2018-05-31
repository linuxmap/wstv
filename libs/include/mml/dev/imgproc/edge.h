/************************************************************************************//**
 *  @file       edge.h
 *
 *  @brief      Image Edge Processor 
 *
 *  @date       2016-08-05 10:15
 *
 ***************************************************************************************/

#ifndef _MML_DEV_EDGE_H_
#define _MML_DEV_EDGE_H_

#include "mml/core/core.h"
#include "mml/imgproc/imgproc_c.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlDevSobel Sobel edge calculation
 * @param src [in] Source Image
 * @param dst_h [out] Destination Horizontal Image
 * @param dst_v [out] Destination Vertical Image
 * @param kt [in] MMLSobelKernelType
 * @param dir [in] Sobel Direction
 * @param size [in] Sobel kernel size
 * @return 0 Sobel status
 */
int mmlDevSobel(const MMLImage * src, MMLImage * dst_h, MMLImage * dst_v, \
        MMLSobelKernelType kt, MMLSobelDirType dir, int size);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* !_MML_EDGE_H_ */
