/*
 * device_main.h
 *
 *  Created on: 2015-4-8
 *      Author: Administrator
 */

#ifndef DEVICE_MAIN_H_
#define DEVICE_MAIN_H_

typedef struct _UPNP
{
	char ip_address[16];
	unsigned short port;
	char desc_doc_name[32];
	char web_dir_path[32];
}stUpnp;


int upnp_device_init(stUpnp *upnp);
int upnp_device_deinit();
int upnp_device_reset(stUpnp *upnp);

#endif /* DEVICE_MAIN_H_ */
