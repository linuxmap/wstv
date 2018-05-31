/*************************************************************************************//**
 *  @file       gmm.h
 *
 *  @brief      Brief description of gmm.h
 *
 *  @date       2016-11-23 17:43
 *         
 **************************************************************************************/


#ifndef _MML_GMM_H_
#define _MML_GMM_H_

#include "mml/core/core.h"
#include "mml/imgproc/imgproc_c.h"

typedef struct _MMLGMMProp {
    MMLSize img_size;
    int model_num;
    float learning_rate;
    float bg_ratio;
    float noise_var;
} MMLGMMProp;

typedef struct _MMLGMMHandle {
    int frame_no;
    MMLGMMProp gmm_prop;
    MMLMemStorage * mem;
} MMLGMMHandle;

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlGMMInit Init GMM module
 * @param prop [in] gmm properties
 * @return h mml GMM handle
 */
MMLGMMHandle * mmlGMMInit(MMLGMMProp prop);

/**
 * mmlGMMInit Init GMM module
 * @param h_gmm [in] mml gmm handle pointer
 * @param src [in] image need to separate the fg & bg
 * @param fg [out] fg image
 * @param bg [out] bg image
 * @return MMLError final status
 */
int mmlGMM(MMLGMMHandle * h_gmm, const MMLImage * src, \
        const MMLImage * fg, const MMLImage * bg);

/**
 * mmlGMMFinal Finalize GMM module
 * @param h_gmm [in] mml gmm handle pointer pointer
 * @return MMLError final status
 */
int mmlGMMFinal(MMLGMMHandle ** h_gmm);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* !_MML_GMM_H_ */

