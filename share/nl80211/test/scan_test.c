#include <stdio.h>

#include "iwlist.h"
#include "iw_80211_scan.h"

int main()
{
	wifiap_t list[128];
	int num = sizeof(list) / sizeof(list[0]);
	int i;

	iw_80211_scan("wlan0", list, &num);

	printf("search num: %d\n", num);

	for (i = 0; i < num; ++i)
	{
		printf("%-3d, ssid: %-32s, signal: %-4d, keystat: %-4d, auth: %-10s, cipher: %-10s\n", 
					i + 1, list[i].name, list[i].quality, list[i].keystat, 
					list[i].iestat[0] == AUTH_TYPE_OPEN ? "WEP-OPEN" :
					list[i].iestat[0] == AUTH_TYPE_SHARED ? "WEP-SHARE" :
					list[i].iestat[0] == AUTH_TYPE_AUTO ? "WEP-AUTO" :
					list[i].iestat[0] == AUTH_TYPE_WPA ? "WPA-PSK" :
					list[i].iestat[0] == AUTH_TYPE_WPA2 ? "WPA2-PSK" : "NONE", 
					list[i].iestat[1] == ENCODE_TYPE_NONE ? "NONE" :
					list[i].iestat[1] == ENCODE_TYPE_WEP ? "WEP" :
					list[i].iestat[1] == ENCODE_TYPE_TKIP ? "TKIP" : "AES");
	}
	
	return 0;
}