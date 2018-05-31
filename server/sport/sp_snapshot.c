/*
 * sp_snapshot.cpp
 *
 *  Created on: 2013Äê12ÔÂ23ÈÕ
 *      Author: lfx  20451250@qq.com
 */
#include <jv_common.h>
#include "sp_snapshot.h"
#include <msnapshot.h>
#include "sp_ifconfig.h"
#include "jv_snapshot.h"


int sp_snapshot(int channelid, const char *fname)
{
	return msnapshot_get_file(0, (char *)fname);
}

const char* sp_snapshot_get_uri(int channelid)
{
	static char uri[128];
	SPEth_t eth;
	sp_ifconfig_eth_get(&eth);

	//sp_snapshot(0, "/progs/html/cgi-bin/snapshot/snap.jpg");
	//sprintf(uri, "http://%s/%s", eth.addr, "cgi-bin/snapshot/snap.jpg");
	sprintf(uri, "http://%s/%s", eth.addr, "cgi-bin/getsnapshot.cgi");

	return uri;
}

int sp_snapshort_get_data(int channelid, unsigned char *pData ,unsigned int len, stSPSnapSize *snap)
{
	stSnapSize size;
	size.nHeight = snap->nHeight;
	size.nWith = snap->nWith;
	return msnapshot_get_data(channelid, pData, len, &size);
}

