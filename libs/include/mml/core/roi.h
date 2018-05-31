/******************************************************************************
 *  @file       roi.h
 *
 *  @brief      Brief description of roi.h
 *
 *  @date       2016-06-02 10:51
 *  @author     liuchen - liuchen@jovision.com
 *
 *****************************************************************************/


#ifndef _MML_CORE_ROI_H_
#define _MML_CORE_ROI_H_

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlSetImageROI Set roi for MMLImage
 * @param img [in] MMLImage which need to set roi
 * @param roi [in] region of interesting
 * @return MMLImage * MMLImage which has been set roi
 */
MMLImage * mmlSetImageROI(MMLImage * img, const MMLRect * roi);

/**
 * mmlResetImageROI Reset roi for MMLImage
 * @param img [in] MMLImage which need to set roi
 * @return MMLImage * MMLImage which has been reset roi
 */
MMLImage * mmlResetImageROI(MMLImage * img);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* !_MML_CORE_ROI_H_ */

