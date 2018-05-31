#include "jv_vtcs.h"

jv_vtcs_func_t g_vtcsfunc;


int jv_vtcs_init()
{
	memset(&g_vtcsfunc, 0, sizeof(g_vtcsfunc));

	return 0;
}



