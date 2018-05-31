/*
 ============================================================================
 Name        : imgproc_c.h
 Author      : Liuchen
 Version     :
 Copyright   : Your copyright notice
 Description : imgproc
 Created on	 : 2016-04-09
 ============================================================================
 */

#ifndef _MML_DEV_IMGPROC_C_H_
#define _MML_DEV_IMGPROC_C_H_

#include "hi_comm_video.h"

#include "mml/dev/imgproc/types_c.h"
#include "mml/dev/imgproc/contours.h"
#include "mml/dev/imgproc/logic.h"
#include "mml/dev/imgproc/filter.h"
#include "mml/dev/imgproc/edge.h"
#include "mml/dev/imgproc/cvt_color.h"
#include "mml/dev/imgproc/gmm.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlDevFrameCopy bind image with HISI Frame
 * @param src [in] Source Frame
 * @param dst [in] Destination Image
 * @return 0 copy status
 */
int mmlDevFrameBindImage(const VIDEO_FRAME_INFO_S * src, MMLImage ** dst);

/**
 * mmlDevCopy copy image
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @return 0 copy status
 */
int mmlDevCopy(const MMLImage * src, MMLImage * dst);

/**
 * mmlDevFrameCopy copy image from HISI Frame
 * @param src [in] Source Frame
 * @param dst [in] Destination Image
 * @return 0 copy status
 */
int mmlDevFrameCopy(const VIDEO_FRAME_INFO_S * src, MMLImage ** dst);

/**
 * mmlDevFrameImageCopy copy image from special image which bind with frame
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @return 0 copy status
 */
int mmlDevFrameImageCopy(const MMLImage * src, MMLImage * dst);

/**
 * mmlDevResize resize image
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param interpolation [in] interpolation method
 * @return 0 calc status
 */
int mmlDevResize(const MMLImage * src, MMLImage * dst, int interpolation);

/**
 * mmlDevVGSResize resize image
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param interpolation [in] interpolation method
 * @return 0 calc status
 */
int mmlDevVGSResize(const VIDEO_FRAME_INFO_S *src, MMLImage * dst);

/**
 * mmlDevAddWeighted add image by weight
 * @param src1 [in] Source Image 1
 * @param alpha [in] weight of Source Image 1
 * @param src2 [in] Source Image 2
 * @param beta [in] weight of Source Image 2
 * @param dst [in] Destination Image
 * @return 0 calc status
 */
int mmlDevAddWeighted(const MMLImage * src1, float alpha, \
        const MMLImage * src2, float beta, \
        MMLImage * dst);

/**
 * mmlDevSubtract calc image dst = src1 - src2
 * @param src1 [in] Source Image 1
 * @param src2 [in] Source Image 2
 * @param dst [in] Destination Image
 * @return 0 calc status
 */
int mmlDevSubtract(const MMLImage * src1, const MMLImage * src2, MMLImage * dst);

/**
 * mmlDevAbsdiff calc image dst = src1 - src2
 * @param src1 [in] Source Image 1
 * @param src2 [in] Source Image 2
 * @param dst [in] Destination Image
 * @return 0 calc status
 */
int mmlDevAbsdiff(const MMLImage * src1, const MMLImage * src2, MMLImage * dst);

/**
 * mmlDevThreshold binarization image by threshold
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param threshold [in] as the word meaning
 * @param maxv [in] binarization max value
 * @param type [in] binarization type
 * @return 0 calc status
 */
int mmlDevThreshold(const MMLImage * src, MMLImage * dst, \
        int32_t threshold, int32_t maxv, MMLThreshType type);

/**
 * mmlDevIntegral calc image intergral
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @return 0 copy status
 */
int mmlDevIntegral(const MMLImage * src, MMLImage * dst);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* ifndef _MML_IMGPROC_C_H_ */
