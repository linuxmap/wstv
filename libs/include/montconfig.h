#ifndef _MONT_CONFIG_H_
#define _MONT_CONFIG_H_

// return 0 if success, other failed
int montconfig_start(const char* ifname);

// return result string if success
const char* montconfig_getconfig();

// return 0 if sucess
int montconfig_stop();

#endif