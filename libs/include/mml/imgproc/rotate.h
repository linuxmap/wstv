/*************************************************************************************//**
 *  @file       rotate.h
 *
 *  @brief      Rotate Image 
 *
 *  @date       2016-08-15 13:02
 *         
 **************************************************************************************/


#ifndef _MML_ROTATE_H_
#define _MML_ROTATE_H_

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlRotate Rotate Image
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param angle [in] Rotate Angle
 * @return 0 Rotate status
 */
int mmlRotate(const MMLImage * src, MMLImage * dst, float angle);

/**
 * mmlQuadrangleSubPix Rotate Image
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @param rotate_mat [in] Rotate Matrix
 * @return 0 QuadrangleSubPix status
 */
int mmlQuadrangleSubPix(const MMLImage * src, \
        MMLImage * dst, MMLMat * rotate_mat);

/**
 * mmlScale2Mat transform scale to rotate mat
 * @param scale [in] Scale
 * @param mat [out] Destination Mat
 * @return 0 transform status
 */
int mmlScale2Mat(double scale, MMLMat * mat);

/**
 * mmlAngle2Mat transform angle to rotate mat
 * @param angle [in] Angle
 * @param mat [out] Destination Mat
 * @return 0 transform status
 */
int mmlAngle2Mat(double angle, MMLMat * mat);

/**
 * mmlShift2Mat transform shift to rotate mat
 * @param shift [in] Shift Coordinate
 * @param mat [out] Destination Mat
 * @return 0 transform status
 */
int mmlShift2Mat(MMLPoint32F shift, MMLMat * mat);

/**
 * mmlMatMul Mat multiply
 * @param src [in] Source Mat
 * @param dst [out] Destination dst
 * @return 0 calc status
 */
int mmlMatCopy(const MMLMat * src, MMLMat * dst);

/**
 * mmlMatMul Mat multiply
 * @param src1 [in] Source Mat 1
 * @param src2 [in] Source Mat 2
 * @param dst [out] Destination dst
 * @return 0 calc status
 */
int mmlMatMul(const MMLMat * src1, const MMLMat * src2, MMLMat * dst);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* !_MML_ROTATE_H_ */

