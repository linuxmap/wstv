/************************************************************************************//**
 *  @file       cvt_color.h
 *
 *  @brief      convert image color space
 *
 *  @date       2016-09-12 16:21
 *
 ***************************************************************************************/

#ifndef _MML_CVT_COLOR_H_
#define _MML_CVT_COLOR_H_

#include "mml/core/core.h"
#include "mml/imgproc/imgproc_c.h"

typedef enum _MMLCvtColorType {
    //YUV 4:2:0 formats family
    MML_YUV2RGB_NV12 = 90,
    MML_YUV2BGR_NV12 = 91,
    MML_YUV2RGB_NV21 = 92,
    MML_YUV2BGR_NV21 = 93,
    MML_YUV420sp2RGB = MML_YUV2RGB_NV21,
    MML_YUV420sp2BGR = MML_YUV2BGR_NV21,
    MML_CVT_COLOR_BUTT,
} MMLCvtColorType;

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlCvtColor Convert Color Space
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param type [in] Convert Color Type
 * @return 0 And status
 */
int mmlCvtColor(const MMLImage * src, MMLImage * dst, MMLCvtColorType type);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* !_MML_CVT_COLOR_H_ */
