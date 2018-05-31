#ifndef _IW_SCAN_H_
#define _IW_SCAN_H_

int iw_80211_scan(const char* ifname, wifiap_t *aplist, int* maxCnt);

#endif