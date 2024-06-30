#pragma once

#include <common/utils/visitor.h>
#include <cstdint>

namespace NesEmulator
{
enum Mode : unsigned;
class Bus;

struct DMCRegister
{
    union Flags
    {
        struct
        {
            uint8_t rate : 4;
            uint8_t unused : 2;
            uint8_t loop : 1;
            uint8_t irqEnabled : 1;
        };

        uint8_t reg = 0x00;
    } flags;

    uint8_t directLoad = 0x00;
    uint8_t sampleAddress = 0x00;
    uint8_t sampleLength = 0x00;

    void Reset();
    void SerializeTo(Common::Utils::IWriteVisitor& visitor) const;
    void DeserializeFrom(Common::Utils::IReadVisitor& visitor);
};

class DMCChannel
{
public:
    void Reset();

    double GetSample();

    void ConnectBus(NesEmulator::Bus* bus) { m_bus = bus; }

    // Return true if there is an interrupt from DMC
    bool Update(double cpuFrequency);

    void Enable(bool enable);
    bool IsEnabled() const { return m_enabled || m_fadeOut; }

    uint16_t Remaining() const { return m_remainingSamples; }

    void WriteData(uint16_t address, uint8_t data, const Mode& mode);
    void SerializeTo(Common::Utils::IWriteVisitor& visitor) const;
    void DeserializeFrom(Common::Utils::IReadVisitor& visitor);

private:
    void Restart();
    // Returns true if remaining sampled reached 0
    bool ReadNewSample();

    void LoadSampleAddress();
    void LoadSampleLength();

    NesEmulator::Bus* m_bus = nullptr;
    DMCRegister m_register;

    // Memory reader
    bool m_sampleBufferIsEmpty = true;
    uint16_t m_currentAddress = 0x0000;
    uint16_t m_remainingSamples = 0x0000;
    uint8_t m_sampleBuffer = 0x00;

    // Output unit
    bool m_enabled = false;
    uint16_t m_currentRate = 0;
    uint8_t m_currentOutput = 0x00;
    uint8_t m_bitShiftRegister = 0x00;
    uint8_t m_bitsRemaining = 0x00;
    uint16_t m_timer = 0;

    bool m_fadeIn = false;
    double m_fadeInValue = 0.0;

    bool m_fadeOut = false;
    double m_fadeOutValue = 0.0;
};
} // namespace NesEmulator