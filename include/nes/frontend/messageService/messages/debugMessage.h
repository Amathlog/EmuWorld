#pragma once

#include <cstdint>
#include <frontend/messageService/message.h>
#include <frontend/messageService/messages/debugPayload.h>

namespace NesEmulatorGL
{
using DebugMessage = TypedMessage<DefaultMessageType::DEBUG, DebugPayload>;

struct GetNametablesMessage : DebugMessage
{
    GetNametablesMessage(uint8_t* data, size_t dataSize, size_t dataCapacity)
        : DebugMessage(DefaultDebugMessageType::READ_NAMETABLES, data, dataSize, dataCapacity)
    {
    }
};
} // namespace NesEmulatorGL