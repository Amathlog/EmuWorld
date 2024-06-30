#include <frontend/messageService/message.h>
#include <frontend/messageService/messages/screenPayload.h>
#include <frontend/messageService/screenMessageService.h>
#include <frontend/screen.h>

using NesEmulatorGL::ScreenMessageService;

bool ScreenMessageService::Push(const Message& message)
{
    if (message.GetType() != DefaultMessageType::SCREEN)
        return true;

    auto payload = reinterpret_cast<const ScreenPayload*>(message.GetPayload());

    switch (payload->m_type)
    {
    case DefaultScreenMessageType::CHANGE_FORMAT:
        m_screen.GetImage().SetImageFormat(payload->m_format);
        break;
    case DefaultScreenMessageType::RESIZE:
        m_screen.OnScreenResized(payload->m_width, payload->m_height);
        break;
    case DefaultScreenMessageType::RENDER:
        m_screen.GetImage().UpdateInternalBuffer(reinterpret_cast<const uint8_t*>(payload->m_dataPtr),
                                                 payload->m_dataSize);
        break;
    }

    return true;
}

bool ScreenMessageService::Pull(Message& message)
{
    if (message.GetType() != DefaultMessageType::SCREEN)
        return true;

    auto payload = reinterpret_cast<ScreenPayload*>(message.GetPayload());

    switch (payload->m_type)
    {
    case DefaultScreenMessageType::GET_FORMAT:
        payload->m_format = m_screen.GetImage().GetImageFormat();
        break;
    case DefaultScreenMessageType::GET_FRAMETIME:
        payload->m_dataPtr = m_screen.GetFrametimes(payload->m_offset, payload->m_dataSize);
        break;
    }

    return true;
}