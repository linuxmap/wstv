/*************************************************************************************//**
 *  @file       types_c.h
 *
 *  @brief      types_c.h for imgproc
 *
 *  @date       2016-05-09 10:18
 *  @author     liuchen - liuchen@jovision.com
 *         
 **************************************************************************************/


#ifndef _MML_DEV_IMGPROC_TYPES_C_H_
#define _MML_DEV_IMGPROC_TYPES_C_H_

#include "mml/core/core.h"
#include "mml/imgproc/imgproc_c.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlDevFlush flush image memory sync with cache
 * @param img [in] Image Wait for Flush
 * @return 0 calc status
 */
int mmlDevFlush(const MMLImage * img);

/**
 * mmlDevMemFlush flush memory storage memory
 * @param img [in] mem Wait for Flush
 * @return 0 calc status
 */
int mmlDevMemFlush(const MMLMemStorage * mem);

/**
 * mmlDev16bitTo8bit Transform Image From 16bit to 8bit
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @return 0 calc status
 */
int mmlDev16bitTo8bit(const MMLImage * src, MMLImage * dst);

/**
 * mmlDevDilate dilate image
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param kt [in] dilate kernel type
 * @param size [in] Dilate kernel size
 * @return 0 calc status
 */
int mmlDevDilate(const MMLImage * src, MMLImage * dst, \
        MMLConvKernelType kt, int size);

/**
 * mmlDevErode Erode image
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param kt [in] Erode kernel type
 * @param size [in] Erode kernel size
 * @return 0 calc status
 */
int mmlDevErode(const MMLImage * src, MMLImage * dst, \
        MMLConvKernelType kt, int size);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* !_MML_IMGPROC_TYPES_C_H_ */

