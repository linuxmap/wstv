/*
 ============================================================================
 Name        : memory.h
 Author      : Liuchen
 Version     :
 Copyright   : Your copyright notice
 Description : memory operate
 Created on	 : 2016-04-18
 ============================================================================
 */

#ifndef _MML_MEMORY_H_
#define _MML_MEMORY_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

// wrapper of malloc
void * mmlMalloc(size_t size);

// wrapper of calloc
void * mmlCalloc(size_t nmemb, size_t size);

// wrapper of free
void mmlFree(void * ptr);

#if defined(_HISI_PLATFORM_)
#define _CACHE_MMZ_

typedef struct _MMLMMZ {
    void * ptr;
    unsigned int ptr_phy;
    size_t size;
} MMLMMZ;

// wrapper of hisi mmz malloc
MMLMMZ mmlHisiMalloc(size_t size);

// wrapper of hisi mmz calloc
MMLMMZ mmlHisiCalloc(size_t nmemb, size_t size);

// wrapper of hisi mmz free
int mmlHisiFree(MMLMMZ ptr);

#endif /* defined(_HISI_PLATFORM_) */

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* ifndef _MML_MEMORY_H_ */
