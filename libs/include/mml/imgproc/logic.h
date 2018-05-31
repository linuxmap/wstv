/*************************************************************************************//**
 *  @file       logic.h
 *
 *  @brief      Brief description of logic.h
 *
 *  @date       2016-08-03 10:03
 *         
 **************************************************************************************/


#ifndef _MML_LOGIC_H_
#define _MML_LOGIC_H_

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * mmlAnd And Operation for Binary Image : dst = src1 & src2
 * @param src1 [in] Source Image 1
 * @param src2 [in] Source Image 2
 * @param dst [in] Destination Image
 * @return 0 And status
 */
int mmlAnd(const MMLImage * src1, const MMLImage * src2, MMLImage * dst);

/**
 * mmlOr Or Operation for Binary Image : dst = src1 | src2
 * @param src1 [in] Source Image 1
 * @param src2 [in] Source Image 2
 * @param dst [in] Destination Image
 * @return 0 Or status
 */
int mmlOr(const MMLImage * src1, const MMLImage * src2, MMLImage * dst);

/**
 * mmlNot Not Operation for Binary Image : dst = ~src
 * @param src [in] Source Image
 * @param dst [in] Destination Image
 * @return 0 Not status
 */
int mmlNot(const MMLImage * src, MMLImage * dst);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* !_MML_LOGIC_H_ */

