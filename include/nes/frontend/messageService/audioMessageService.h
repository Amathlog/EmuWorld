#pragma once

#include <frontend/messageService/messageService.h>

namespace NesEmulatorGL
{
class AudioSystem;

class AudioMessageService : public IMessageService
{
public:
    AudioMessageService(AudioSystem& audioSystem)
        : m_audioSystem(audioSystem)
    {
    }

    bool Push(const Message& message) override;
    bool Pull(Message& message) override;

private:
    AudioSystem& m_audioSystem;
};
} // namespace NesEmulatorGL