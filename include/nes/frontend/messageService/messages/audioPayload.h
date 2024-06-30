#pragma once

#include <frontend/messageService/message.h>
#include <string>

namespace NesEmulatorGL
{
using AudioMessageType = uint32_t;

enum DefaultAudioMessageType : AudioMessageType
{
    ENABLE_AUDIO,
    GET_ENABLE
};

class AudioPayload : public Payload
{
public:
    AudioPayload(AudioMessageType type, bool data)
        : m_type(type)
        , m_data(data)
    {
    }

    AudioMessageType m_type;
    bool m_data;
};
} // namespace NesEmulatorGL