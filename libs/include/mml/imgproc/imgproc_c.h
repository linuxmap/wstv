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

#ifndef _MML_IMGPROC_C_H_
#define _MML_IMGPROC_C_H_

#include "mml/imgproc/types_c.h"
#include "mml/imgproc/contours.h"
#include "mml/imgproc/logic.h"
#include "mml/imgproc/filter.h"
#include "mml/imgproc/edge.h"
#include "mml/imgproc/cvt_color.h"
#include "mml/imgproc/gmm.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlCopy copy image
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @return 0 copy status
 */
int mmlCopy(const MMLImage * src, MMLImage * dst);

/**
 * mmlCopyByMask copy image by mask
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param mask [in] Mask Image
 * @return 0 copy status
 */
int mmlCopyByMask(const MMLImage * src, MMLImage * dst, \
        const MMLImage * mask);

typedef enum _MMLInterpolation {
    MML_INTER_NN = 0,
    MML_INTER_LINEAR = 1,
    MML_INTER_CUBIC = 2,
    MML_INTER_AREA = 3,
    MML_INTER_LANCZOS4 = 4,
    MML_INTER_BUTT,
} MMLInterpolation;

/**
 * mmlResize resize image
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param interpolation [in] interpolation method
 * @return 0 calc status
 */
int mmlResize(const MMLImage * src, MMLImage * dst, int interpolation);

/**
 * mmlAddWeighted add image by weight
 * @param src1 [in] Source Image 1
 * @param alpha [in] weight of Source Image 1
 * @param src2 [in] Source Image 2
 * @param beta [in] weight of Source Image 2
 * @param dst [in] Destination Image
 * @return 0 calc status
 */
int mmlAddWeighted(const MMLImage * src1, float alpha, \
        const MMLImage * src2, float beta, \
        MMLImage * dst);

/**
 * mmlSubtract calc image dst = src1 - src2
 * @param src1 [in] Source Image 1
 * @param src2 [in] Source Image 2
 * @param dst [in] Destination Image
 * @return 0 calc status
 */
int mmlSubtract(const MMLImage * src1, const MMLImage * src2, MMLImage * dst);

/**
 * mmlAbsdiff calc image dst = src1 - src2
 * @param src1 [in] Source Image 1
 * @param src2 [in] Source Image 2
 * @param dst [in] Destination Image
 * @return 0 calc status
 */
int mmlAbsdiff(const MMLImage * src1, const MMLImage * src2, MMLImage * dst);

/**
 * mmlAdd calc image dst = src1 + src2
 * @param src1 [in] Source Image 1
 * @param src2 [in] Source Image 2
 * @param white [in] white Image
 * @param dst [out] Destination Image
 * @param assist [inout] assistant Image
 * @return 0 calc status
 */
int mmlAdd(const MMLImage * src1, const MMLImage * src2, MMLImage * dst, \
        MMLImage * white, MMLImage * assist);

typedef enum _MMLThreshType {
    MML_THRESH_BINARY = 0,
    MML_THRESH_BINARY_INV,
    MML_THRESH_TRUNC,
    MML_THRESH_OTSU,
    MML_THRESH_BUTT,
} MMLThreshType;

typedef enum _MMLAdaptiveThreshType {
    MML_ADAPTIVE_THRESH_MEAN_C = 0,
    MML_ADAPTIVE_THRESH_GAUSSIAN_C,
    MML_ADAPTIVE_THRESH_BUTT = 0,
} MMLAdaptiveThreshType;

/**
 * mmlThreshold binarization image by threshold
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param threshold [in] as the word meaning
 * @param maxv [in] binarization max value
 * @param type [in] binarization type
 * @return 0 calc status
 */
int mmlThreshold(const MMLImage * src, MMLImage * dst, \
        int32_t threshold, int32_t maxv, MMLThreshType type);

/**
 * mmlGetOTSUThreshold Get OTSU threshold
 * @param src [in] Source Image
 * @return otsu threshold
 */
int mmlGetOTSUThreshold(const MMLImage * src);

/**
 * mmlAdaptiveThreshold adaptive binarization image by threshold
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param maxv [in] binarization max value
 * @param adp_type [in] adaptive binarization type
 * @param thresh_type [in] binarization type
 * @param C [in] C
 * @return 0 calc status
 */
int mmlAdaptiveThreshold(const MMLImage * src, MMLImage * dst, \
        int32_t maxv, MMLAdaptiveThreshType adp_type, \
        MMLThreshType thresh_type, int blockSize, int32_t C);

/**
 * mmlIntegral calc image intergral
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @return 0 copy status
 */
int mmlIntegral(const MMLImage * src, MMLImage * dst);

/**
 * mmlWinAvgOnIntegral calc windows average image depend on integral image
 *                      Use mmlIntegral + mmlWinAvgByIntegral to Implement
 *                      Average Filter
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param block_size [in] Windows Block Size
 * @return 0 copy status
 */
int mmlWinAvgOnIntegral(const MMLImage * src, MMLImage * dst, int block_size);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* ifndef _MML_IMGPROC_C_H_ */
