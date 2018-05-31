/*************************************************************************************//**
 *  @file       types_c.h
 *
 *  @brief      types_c.h for imgproc
 *
 *  @date       2016-05-09 10:18
 *  @author     liuchen - liuchen@jovision.com
 *         
 **************************************************************************************/


#ifndef _MML_IMGPROC_TYPES_C_H_
#define _MML_IMGPROC_TYPES_C_H_

#include "mml/core/core.h"

typedef enum _MMLConvKernelType {
    MML_KERNEL_CROSS,
    MML_KERNEL_RECT,
    MML_KERNEL_ELLIPSE,
    MML_KERNEL_HOR_LINE,
    MML_KERNEL_VER_LINE,
    MML_KERNEL_BUTT,
} MMLConvKernelType;

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlDilate dilate image
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param kt [in] dilate kernel type
 * @param size [in] Dilate kernel size
 * @param iteration [in] iteration times
 * @return 0 calc status
 */
int mmlDilate(const MMLImage * src, MMLImage * dst, \
        MMLConvKernelType kt, int size, \
        int iteration);

/**
 * mmlErode Erode image
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param kt [in] Erode kernel type
 * @param size [in] Erode kernel size
 * @param iteration [in] iteration times
 * @return 0 calc status
 */
int mmlErode(const MMLImage * src, MMLImage * dst, \
        MMLConvKernelType kt, int size, \
        int iteration);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* !_MML_IMGPROC_TYPES_C_H_ */

