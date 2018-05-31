#ifndef _UTL_ALGORITHM_H_
#define _UTL_ALGORITHM_H_


typedef int (*FuncCmp)(const void* a, const void* b);

void utl_qsort(void* pItemArray, unsigned int ItemCnt, unsigned int ItemSize, FuncCmp Function);

#endif

