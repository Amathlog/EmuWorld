#pragma once

#include <frontend/messageService/message.h>

namespace NesEmulatorGL
{
using DebugMessageType = uint32_t;

enum DefaultDebugMessageType : DebugMessageType
{
    READ_NAMETABLES,
};

class DebugPayload : public GenericPayload
{
public:
    DebugPayload(DebugMessageType type, uint8_t* data, size_t dataSize, size_t dataCapacity)
        : GenericPayload(data, dataSize, dataCapacity)
        , m_type(type)
    {
    }

    DebugMessageType m_type;
};
} // namespace NesEmulatorGL