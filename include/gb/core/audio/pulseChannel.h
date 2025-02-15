#pragma once

#include <common/utils/visitor.h>
#include <core/audio/registers.h>

#include <cstdint>
#include <string>

namespace GBEmulator
{
class APU;

class PulseOscillator
{
public:
    PulseOscillator(double sampleRate);
    double Tick();

    void SetFrequency(double freq) { m_freq = freq; };
    void SetDuty(double duty) { m_duty = duty; }
    void Reset()
    {
        m_phase = 0.0;
        m_duty = 0.5;
    }

private:
    double m_phase = 0.0;
    double m_freq = 0.0;
    double m_duty = 0.5;
    double m_sampleRate = 0.0;
};

class PulseChannel
{
public:
    PulseChannel(int number);
    ~PulseChannel() = default;

    void Update(uint8_t divCounter);
    void Reset();

    bool IsEnabled() const { return m_enabled; }
    void SetEnable(bool enabled) { m_enabled = enabled; }

    void WriteByte(uint16_t addr, uint8_t data, const APU* apu);
    uint8_t ReadByte(uint16_t addr) const;
    bool IsDACOn() const { return !!(m_volumeReg.reg & 0xF8); }

    void SerializeTo(Common::Utils::IWriteVisitor& visitor) const;
    void DeserializeFrom(Common::Utils::IReadVisitor& visitor);

    double GetSample();

private:
    void UpdateFreq();
    void CheckOverflow(uint16_t newFreq);
    void Restart();
    void Sweep();

    int m_number;
    SweepRegister m_sweepReg;
    WavePatternRegister m_waveReg;
    VolumeEnveloppeRegister m_volumeReg;
    uint8_t m_freqLsb = 0x00;
    FrequencyHighRegister m_freqMsbReg;

    PulseOscillator m_oscillator;

    uint16_t m_combinedFreq = 0x0000;
    double m_frequency = 0.0;
    uint8_t m_lengthCounter = 0;
    uint8_t m_sweepCounter = 0;
    uint8_t m_volumeCounter = 0;
    uint8_t m_volume = 0;

    bool m_enabled = false;
    bool m_frequencyChanged = false;
    bool m_dutyChanged = false;
};
} // namespace GBEmulator