/*
 ============================================================================
 Name        : core.h
 Author      : Liuchen
 Version     :
 Copyright   : Your copyright notice
 Description : core
 Created on	 : 2016-04-09
 ============================================================================
 */

#ifndef _MML_CORE_H_
#define _MML_CORE_H_

#include <stdint.h>

#include <mml/core/log.h>
#include <mml/core/types_c.h>
#include <mml/core/memory.h>
#include <mml/core/error.h>
#include <mml/core/roi.h>

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

typedef struct _MMLImageHead {
    uint16_t width;
    uint16_t height;
    uint16_t color;
    uint16_t reserve[29];
} MMLImageHead;

/**
 * mmlLoadMMLImage load mml image
 * @param name [in] MMLImage file name
 * @return MMLImage * MMLImage pointer which loaded
 */
MMLImage * mmlLoadMMLImage(const char * name);

/**
 * mmlSaveMMLImage save mml image
 * @param img [in] MMLImage
 * @return int save status
 */
int mmlSaveMMLImage(const char* name, MMLImage * img);

/**
 * mmlCountNonZero count image non zero pixel number
 * @param img [in] image which need count non zero pixel
 * @return number non zero pixel number
 */
int mmlCountNonZero(const MMLImage * img);

/**
 * mmlSetImage Set mml image with value
 * @param img [in] MMLImage
 * @param value [in] set MMLImage to this value
 * @return int save status
 */
MMLImage * mmlSetImage(MMLImage * img, uint32_t value);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* ifndef _MML_CORE_H_ */
