/************************************************************************************//**
 *  @file       cvt_color.h
 *
 *  @brief      convert image color space
 *
 *  @date       2016-09-12 16:21
 *
 ***************************************************************************************/

#ifndef _MML_DEV_CVT_COLOR_H_
#define _MML_DEV_CVT_COLOR_H_

#include "mml/core/core.h"
#include "mml/imgproc/imgproc_c.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlDevCvtColor Convert Color Space
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param type [in] Convert Color Type
 * @return 0 And status
 */
int mmlDevCvtColor(const MMLImage * src, MMLImage * dst, MMLCvtColorType type);

/**
 * mmlDevFrameCvtColor Convert Color Space
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param type [in] Convert Color Type
 * @return 0 And status
 */
int mmlDevFrameCvtColor(const VIDEO_FRAME_INFO_S * src, MMLImage * dst, \
        MMLCvtColorType type);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* !_MML_CVT_COLOR_H_ */
