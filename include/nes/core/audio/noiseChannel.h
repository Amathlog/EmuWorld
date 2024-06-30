#pragma once

#include <common/utils/visitor.h>
#include <core/audio/enveloppe.h>
#include <core/constants.h>
#include <cstdint>

namespace NesEmulator
{
struct NoiseRegister
{
    uint8_t enveloppeLoop;
    uint8_t constantVolume;
    uint8_t volumeEnveloppe;
    uint8_t mode;
    int16_t noisePeriod;
    uint8_t lengthCounterLoad;
    bool noisePeriodChanged;

    void Reset();
    void SerializeTo(Common::Utils::IWriteVisitor& visitor) const;
    void DeserializeFrom(Common::Utils::IReadVisitor& visitor);
    void SetNoisePeriod(uint8_t index, Mode mode);
};

class NoiseOscillator
{
public:
    NoiseOscillator();
    void Reset();
    void SetFrequency(double freq);
    void SetMode(bool mode) { m_bit6Mode = mode; }

    double Tick();

    double m_elaspedTime = 0.0;
    uint16_t m_shiftRegister = 1;
    double m_realSampleDuration = 0.0;
    double m_sampleDuration = 0.0;
    bool m_bit6Mode = false;
};

class NoiseChannel
{
public:
    NoiseChannel() = default;
    ~NoiseChannel() = default;

    void Reset();
    void Clock(bool isEnabled);
    void ClockEnveloppe();
    void Update(double cpuFrequency);

    void SetMode(bool mode) { m_oscillator.SetMode(mode); }

    double GetSample();

    bool IsEnabled() const { return m_enabled; }

    NoiseRegister& GetRegister() { return m_register; }
    Enveloppe& GetEnveloppe() { return m_enveloppe; }

    void ReloadCounter();
    uint8_t GetCounter() const { return m_lengthCounter; }

    void SerializeTo(Common::Utils::IWriteVisitor& visitor) const;
    void DeserializeFrom(Common::Utils::IReadVisitor& visitor);

private:
    NoiseRegister m_register;
    Enveloppe m_enveloppe;

    NoiseOscillator m_oscillator;

    uint8_t m_lengthCounter = 0;
    int16_t m_timer = 0;
    float m_currentOutput = 0.0f;
    bool m_enabled = false;
};
} // namespace NesEmulator