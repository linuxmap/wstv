/*
 * onvif-main.h
 *
 *  Created on: 2013-12-12
 *      Author: lfx
 */

#ifndef ONVIF_MAIN_H_
#define ONVIF_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
	char device_service_fmt[128]; //¿‡À∆’‚—˘£∫"http://%s/onvif/device_service"
	int  ovfPort;
	FILE *dbgLog;

}OnvifParam_t;

int onvif_run_bywebserver(OnvifParam_t *param);

int onvif_run(OnvifParam_t *param);

int onvif_exit(void);

int onvif_dataHandle(char* buffer,int ilen);

int onvif_sendhello();

int	onvif_reset();

int onvif_test(const char *xmlName);

#ifdef __cplusplus
}
#endif

#endif /* ONVIF_MAIN_H_ */
