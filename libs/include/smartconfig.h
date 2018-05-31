#ifndef _SMART_CONFIG_H_
#define _SMART_CONFIG_H_

// return 0 if success, other failed
int smartconfig_start(char* ifname);

// return result string if success
char* smartconfig_getconfig();

// return 0 if sucess
int smartconfig_stop();

#endif