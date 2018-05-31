/*************************************************************************************//**
 *  @file       gmm.h
 *
 *  @brief      Brief description of gmm.h
 *
 *  @date       2016-11-23 17:43
 *         
 **************************************************************************************/


#ifndef _MML_DEV_GMM_H_
#define _MML_DEV_GMM_H_

#include "mml/core/core.h"
#include "mml/imgproc/imgproc_c.h"

#include <hi_ive.h>

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlDevGMMPropCheck check gmm prop valid or not
 * @param prop [in] gmm properties
 * @return result of check
 */
int mmlDevGMMPropCheck(MMLGMMProp prop);

/**
 * mmlDevGMMInit Init GMM module
 * @param prop [in] gmm properties
 * @return h mml GMM handle
 */
MMLGMMHandle * mmlDevGMMInit(MMLGMMProp prop);

/**
 * mmlDevGMMInit Init GMM module
 * @param h_gmm [in] mml gmm handle pointer
 * @param src [in] image need to separate the fg & bg
 * @param fg [out] fg image
 * @param bg [out] bg image
 * @return MMLError final status
 */
int mmlDevGMM(MMLGMMHandle * h_gmm, const MMLImage * src, \
        const MMLImage * fg, const MMLImage * bg);

/**
 * mmlDevGMMFinal Finalize GMM module
 * @param h_gmm [in] mml gmm handle pointer pointer
 * @return MMLError final status
 */
int mmlDevGMMFinal(MMLGMMHandle ** h_gmm);

/**
 * mmlDevGMMProp2Ctrl transform gmm properties to ctrl param
 * @param h_gmm [in] mml gmm handle pointer
 * @param d_ctrl [out] mml gmm ctrl param
 * @return MMLError final status
 */
int mmlDevGMMProp2Ctrl(MMLGMMHandle * h_gmm, IVE_GMM_CTRL_S * d_ctrl);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* !_MML_DEV_GMM_H_ */

