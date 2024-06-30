#pragma once

#include <common/utils/visitor.h>
#include <cstdint>

namespace NesEmulator
{
struct TriangleRegister
{
    uint8_t control;
    uint8_t linearCounterLoad;
    uint16_t timer;
    uint8_t lengthCounterLoad;

    void SerializeTo(Common::Utils::IWriteVisitor& visitor) const;
    void DeserializeFrom(Common::Utils::IReadVisitor& visitor);
    void Reset();
};

class TriangleOscillator
{
public:
    TriangleOscillator() = default;
    double Tick();

    void SetFrequency(double freq);
    void Reset() { m_phase = 0.0; }

private:
    double m_phase = 0.0;
    double m_freq = 0.0;
    double m_phaseIncrement = 0.0f;
};

class TriangleChannel
{
public:
    TriangleChannel() = default;
    ~TriangleChannel() = default;

    void Reset();
    void ClockLinear(bool isEnabled);
    void ClockLength(bool isEnabled);
    void Update(double cpuFrequency);

    TriangleRegister& GetRegister() { return m_register; }

    void SetLinearControlFlag(uint8_t value) { m_linearControlFlag = value; }
    void ReloadCounter();
    uint8_t GetCounter() const { return m_lengthCounter; }

    void SerializeTo(Common::Utils::IWriteVisitor& visitor) const;
    void DeserializeFrom(Common::Utils::IReadVisitor& visitor);

    double GetSample();

    bool IsEnabled() const { return m_enableValue; }

private:
    TriangleRegister m_register;

    TriangleOscillator m_oscillator;

    double m_frequency = 0.0;
    double m_enableValue = 0.0;
    uint8_t m_linearCounter = 0;
    uint8_t m_lengthCounter = 0;
    uint8_t m_linearControlFlag = 0;
};
} // namespace NesEmulator