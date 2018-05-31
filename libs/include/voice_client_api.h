#ifndef VOICE_CLIENT_API_H_
#define VOICE_CLIENT_API_H_

#include "voice_client_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

int VoiceClient_Init();
int VoiceClient_RegisterEventCallback(VoiceClientEventCallback event_callback, VoiceClientVoiceCallback voice_callback);
int VoiceClient_BuildVoiceChannel(const char *server_ip, int server_port, const char *dev_guid);

#ifdef __cplusplus
}
#endif

#endif

