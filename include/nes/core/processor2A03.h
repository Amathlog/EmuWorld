#pragma once
#include <common/utils/visitor.h>
#include <core/audio/circularBuffer.h>
#include <core/audio/dmcChannel.h>
#include <core/audio/filter_wrapper.h>
#include <core/audio/noiseChannel.h>
#include <core/audio/pulseChannel.h>
#include <core/audio/triangleChannel.h>
#include <core/constants.h>
#include <core/serializable.h>
#include <cstdint>

namespace NesEmulator
{
class Bus;

union APUStatus
{
    struct
    {
        uint8_t enableLengthCounterPulse1 : 1;
        uint8_t enableLengthCounterPulse2 : 1;
        uint8_t enableLengthCounterTriangle : 1;
        uint8_t enableLengthCounterNoise : 1;
        uint8_t enableActivedmc : 1;
        uint8_t unused : 1;
        uint8_t frameInterrupt : 1;
        uint8_t dmcInterrupt : 1;
    };

    uint8_t flags;
};

union FrameCounter
{
    struct
    {
        uint8_t unused : 6;
        uint8_t irqInhibit : 1;
        uint8_t mode : 1;
    };

    uint8_t flags;
};

class Processor2A03 : public ISerializable
{
public:
    Processor2A03();
    ~Processor2A03() = default;

    void Clock();
    bool ShouldIRQ() { return m_statusRegister.frameInterrupt > 0 || m_statusRegister.dmcInterrupt > 0; }

    void ConnectBus(NesEmulator::Bus* bus) { m_dmcChannel.ConnectBus(bus); }

    void Stop();

    void WriteCPU(uint16_t addr, uint8_t data);
    uint8_t ReadCPU(uint16_t addr);

    void Reset();

    void SerializeTo(Common::Utils::IWriteVisitor& visitor) const override;
    void DeserializeFrom(Common::Utils::IReadVisitor& visitor) override;

    void FillSamples(float* outData, unsigned int numFrames, unsigned int numChannels);
    void SetMode(Mode mode) { m_mode = mode; }
    void SampleRequested();

    // Will fill 128 samples (64 sampels left and right) if they are ready.
    bool FillSamplesIfReady(float* outData);

    void SetEnable(bool enable);

private:
    PulseChannel m_pulseChannel1;
    PulseChannel m_pulseChannel2;
    TriangleChannel m_triangleChannel;
    NoiseChannel m_noiseChannel;
    DMCChannel m_dmcChannel;
    APUStatus m_statusRegister;
    FrameCounter m_frameCounterRegister;

    std::array<float, 128> m_internalBuffer;
    size_t m_bufferPtr = 0;
    CircularBuffer<float> m_circularBuffer;
    bool m_samplesReady = false;

    size_t m_frameClockCounter = 0;
    Mode m_mode; // NTSC or PAL

    FO_HPF m_highPassFilter1;
    FO_HPF m_highPassFilter2;
    FO_LPF m_lowPassFilter;

    bool m_enable;
};
} // namespace NesEmulator