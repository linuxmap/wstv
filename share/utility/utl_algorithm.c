#include <string.h>
#include "utl_algorithm.h"


void utl_qsort(void* pItemArr, unsigned int ItemCnt, unsigned int ItemSize, FuncCmp Function)
{
	char key[ItemSize];
	char (*pItem)[ItemSize] = pItemArr;
	int i = 0, j = ItemCnt - 1;

	if ((pItemArr == NULL) || (ItemCnt < 2) || (ItemSize < 1) || (Function == NULL))
	{
		return;
	}

	memcpy(key, &pItem[i], ItemSize);
	while (i < j)
	{
		while ((i < j) && (Function(&pItem[j], key) >= 0))
			--j;
		memcpy(&pItem[i], &pItem[j], ItemSize);

		while ((i < j) && (Function(&pItem[i], key) <= 0))
			++i;
		memcpy(&pItem[j], &pItem[i], ItemSize);
	}
	memcpy(&pItem[i], key, ItemSize);

	if (i > 1)
		utl_qsort(pItemArr, i, ItemSize, Function);
	if (i < ItemCnt - 2)
		utl_qsort(&pItem[i + 1], ItemCnt - 1 - i, ItemSize, Function);
}

