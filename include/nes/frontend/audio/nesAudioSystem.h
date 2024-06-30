#pragma once

#include <core/bus.h>
#include <frontend/audio/audioSystem.h>

namespace NesEmulatorGL
{
class NesAudioSystem : public AudioSystem
{
public:
    NesAudioSystem(NesEmulator::Bus& bus, bool syncWithAudio, unsigned nbChannels = 2, unsigned bufferFrames = 256);
    ~NesAudioSystem() = default;

    int RenderCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime,
                       RtAudioStreamStatus status, void* userData) override;

private:
    NesEmulator::Bus& m_bus;
    bool m_syncWithAudio;
};
} // namespace NesEmulatorGL