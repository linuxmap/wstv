#ifndef VOICE_CLIENT_DEFINES_H_
#define VOICE_CLIENT_DEFINES_H_

// event type
enum
{
    EVENT_ON_BUILD_VOICE_CHANNEL = 1,
    EVENT_ON_START_SEND_VOICE = 2,
    EVENT_ON_END_SEND_VOICE = 3,
    EVENT_ON_CLOSE = 4
};

typedef void (*VoiceClientEventCallback)(int even_type, int ret);
typedef void (*VoiceClientVoiceCallback)(const char *buffer, int length);

#endif

