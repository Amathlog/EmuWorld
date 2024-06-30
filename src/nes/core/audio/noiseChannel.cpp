#include "core/constants.h"
#include <core/audio/noiseChannel.h>

using NesEmulator::NoiseChannel;
using NesEmulator::NoiseOscillator;
using NesEmulator::NoiseRegister;

//////////////////////////////////////////////////////
// Noise Register
//////////////////////////////////////////////////////

void NoiseRegister::Reset()
{
    enveloppeLoop = 0;
    constantVolume = 0;
    volumeEnveloppe = 0;
    mode = 0;
    noisePeriod = 0;
    lengthCounterLoad = 0;
    noisePeriodChanged = true;
}

void NoiseRegister::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    visitor.WriteValue(enveloppeLoop);
    visitor.WriteValue(constantVolume);
    visitor.WriteValue(volumeEnveloppe);
    visitor.WriteValue(mode);
    visitor.WriteValue(noisePeriod);
    visitor.WriteValue(lengthCounterLoad);
}

void NoiseRegister::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    visitor.ReadValue(enveloppeLoop);
    visitor.ReadValue(constantVolume);
    visitor.ReadValue(volumeEnveloppe);
    visitor.ReadValue(mode);
    visitor.ReadValue(noisePeriod);
    visitor.ReadValue(lengthCounterLoad);
}

void NoiseRegister::SetNoisePeriod(uint8_t index, Mode _mode)
{
    auto noisePeriodTable = _mode == Mode::PAL ? Cst::APU_NOISE_PERIOD_PAL : Cst::APU_NOISE_PERIOD_NTSC;
    noisePeriod = noisePeriodTable[index & 0x0F];
    noisePeriodChanged = true;
}

//////////////////////////////////////////////////////
// Noise Oscillator
//////////////////////////////////////////////////////
NoiseOscillator::NoiseOscillator() { m_realSampleDuration = 1.0 / NesEmulator::Cst::SAMPLE_RATE; }

void NoiseOscillator::Reset()
{
    m_shiftRegister = 1;
    m_sampleDuration = 0.0;
    m_elaspedTime = 0.0;
    m_bit6Mode = false;
}

void NoiseOscillator::SetFrequency(double freq)
{
    if (freq == 0.0)
    {
        m_sampleDuration = 0.0;
    }
    else
    {
        m_sampleDuration = 1.0 / freq;
    }
}

double NoiseOscillator::Tick()
{
    double value = m_shiftRegister & 0x0001 ? -1.0 : 1.0;
    m_elaspedTime += m_realSampleDuration;
    if (m_elaspedTime >= m_sampleDuration)
    {
        m_elaspedTime -= m_sampleDuration;
        uint8_t shift = m_bit6Mode ? 6 : 1;
        uint16_t otherFeedback = (m_shiftRegister >> shift) & 0x0001;
        uint16_t feedback = (m_shiftRegister ^ otherFeedback) & 0x0001;
        m_shiftRegister = (feedback << 14) | (m_shiftRegister >> 1);
    }

    return value;
}

//////////////////////////////////////////////////////
// Noise Channel
//////////////////////////////////////////////////////

void NoiseChannel::Clock(bool isEnabled)
{
    bool halt = m_register.enveloppeLoop;
    if (!isEnabled)
    {
        m_lengthCounter = 0;
    }
    else if (m_lengthCounter > 0 && !halt)
    {
        m_lengthCounter--;
    }
}

void NoiseChannel::ClockEnveloppe() { m_enveloppe.Clock(m_register.enveloppeLoop); }

void NoiseChannel::Update(double cpuFrequency)
{
    float temp;
    if (m_lengthCounter > 0 && m_register.noisePeriod > 0)
    {
        m_currentOutput = (float)(m_enveloppe.output) / 15.0f;
        m_enabled = true;
    }
    else
    {
        m_currentOutput = 0.0f;
        m_enabled = false;
    }

    if (m_register.noisePeriodChanged && m_register.noisePeriod > 0)
    {
        m_register.noisePeriodChanged = false;
        double newFrequency = cpuFrequency / ((double)m_register.noisePeriod);
        m_oscillator.SetFrequency(newFrequency);
    }
}

double NoiseChannel::GetSample() { return m_enabled ? (double)m_currentOutput * m_oscillator.Tick() : 0.0; }

void NoiseChannel::Reset()
{
    m_register.Reset();
    m_enveloppe.Reset();
    m_lengthCounter = 0;
    m_timer = 0;
    m_currentOutput = 0.0f;

    m_oscillator.Reset();
}

void NoiseChannel::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    m_register.SerializeTo(visitor);
    m_enveloppe.SerializeTo(visitor);
    visitor.WriteValue(m_lengthCounter);
    visitor.WriteValue(m_timer);
}

void NoiseChannel::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    m_register.DeserializeFrom(visitor);
    m_enveloppe.DeserializeFrom(visitor);
    visitor.ReadValue(m_lengthCounter);
    visitor.ReadValue(m_timer);
}

void NoiseChannel::ReloadCounter() { m_lengthCounter = Cst::APU_LENGTH_TABLE[m_register.lengthCounterLoad]; }