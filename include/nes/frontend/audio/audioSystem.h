#pragma once

#include <RtAudio.h>
#include <frontend/messageService/audioMessageService.h>

namespace NesEmulatorGL
{
class AudioSystem
{
public:
    AudioSystem(unsigned nbChannels = 2, unsigned bufferFrames = 256);

    virtual ~AudioSystem();

    bool Initialize();
    bool Shutdown();
    void Enable(bool value);
    bool IsEnabled() const { return m_enabled; }

    virtual int RenderCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime,
                               RtAudioStreamStatus status, void* userData) = 0;

protected:
    unsigned m_nbChannels = 2;
    unsigned m_bufferFrames = 256;
    AudioMessageService m_messageService;

    bool m_enabled = true;
    bool m_initialized = false;
    RtAudio* m_dac = nullptr;
};
} // namespace NesEmulatorGL