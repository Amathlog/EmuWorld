#include <core/audio/dmcChannel.h>
#include <core/bus.h>
#include <core/constants.h>

using NesEmulator::DMCChannel;
using NesEmulator::DMCRegister;

void DMCRegister::Reset()
{
    flags.reg = 0x00;
    directLoad = 0x00;
    sampleAddress = 0x00;
    sampleLength = 0x00;
}

void DMCRegister::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    visitor.WriteValue(flags.reg);
    visitor.WriteValue(directLoad);
    visitor.WriteValue(sampleAddress);
    visitor.WriteValue(sampleLength);
}

void DMCRegister::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    visitor.ReadValue(flags.reg);
    visitor.ReadValue(directLoad);
    visitor.ReadValue(sampleAddress);
    visitor.ReadValue(sampleLength);
}

void DMCChannel::Reset()
{
    m_register.Reset();
    m_currentRate = 0;

    m_sampleBufferIsEmpty = true;
    m_currentAddress = 0x0000;
    m_remainingSamples = 0x0000;
    m_sampleBuffer = 0x00;

    m_enabled = false;
    m_currentRate = 0;
    m_currentOutput = 0x00;
    m_bitShiftRegister = 0x00;
    m_bitsRemaining = 0x00;

    m_timer = 0;
}

double DMCChannel::GetSample()
{
    constexpr double fadeInIncrement = 0.025;
    constexpr double fadeOutDecrement = 0.025;

    if (!m_enabled && !m_fadeOut)
        return 0.0;

    double sample = ((double)(m_currentOutput)-64.0) / 64.0;

    if (m_fadeIn)
    {
        sample *= m_fadeInValue;
        m_fadeInValue += fadeInIncrement;
        if (m_fadeInValue >= 1.0)
        {
            m_fadeIn = false;
        }
    }

    if (m_fadeOut)
    {
        sample *= m_fadeOutValue;
        m_fadeOutValue -= fadeOutDecrement;
        if (m_fadeOutValue <= 0.0)
        {
            m_fadeOut = false;
        }
    }

    return sample;
}

bool DMCChannel::Update(double cpuFrequency)
{
    // Always try to read a new sample.
    bool shouldIRQ = ReadNewSample();

    // If we are silenced and there is no sample available
    // nothing to do.
    if (!m_enabled && m_sampleBufferIsEmpty)
        return shouldIRQ;

    // Clock the timer and update if we reaches the current rate
    if (++m_timer >= m_currentRate)
    {
        m_timer = 0;

        // Only update the output level if we are not silenced
        if (m_enabled)
        {
            // If the bit 0 is set, increase by 2. Decrease by 2 otherwise
            // Can't go outside 0-127 range
            bool increaseOutput = (m_bitShiftRegister & 0x01) > 0;
            if (increaseOutput && m_currentOutput <= 125)
            {
                m_currentOutput += 2;
            }
            else if (!increaseOutput && m_currentOutput >= 2)
            {
                m_currentOutput -= 2;
            }
        }

        if (m_bitsRemaining > 0)
        {
            m_bitShiftRegister >>= 1;
            m_bitsRemaining--;
        }
    }

    // At this point, check if we have a new cycle to begin
    if (m_bitsRemaining == 0)
    {
        m_bitsRemaining = 8;

        // Load the new sample
        if (!m_sampleBufferIsEmpty)
        {
            if (!m_enabled)
            {
                m_fadeIn = true;
                m_fadeInValue = 0.0;
            }

            m_bitShiftRegister = m_sampleBuffer;
            m_enabled = true;
            m_sampleBufferIsEmpty = true;
        }
        // Otherwise silence the channel.
        else
        {
            if (m_enabled)
            {
                m_fadeOut = true;
                m_fadeOutValue = 1.0;
            }

            m_enabled = false;
        }
    }

    return shouldIRQ;
}

void DMCChannel::Enable(bool enable)
{
    if (!enable)
    {
        // Will silence by itself
        m_remainingSamples = 0;
    }
    else
    {
        // Restart but still wait for the sample buffer to empty.
        Restart();
    }
}

void DMCChannel::Restart()
{
    LoadSampleAddress();
    LoadSampleLength();
    m_sampleBufferIsEmpty = true;
}

bool DMCChannel::ReadNewSample()
{
    if (!m_sampleBufferIsEmpty || m_remainingSamples == 0)
        return false;

    m_sampleBuffer = m_bus->ReadCPU(m_currentAddress);
    m_sampleBufferIsEmpty = false;
    m_remainingSamples--;

    if (m_remainingSamples == 0 && m_register.flags.loop != 0)
    {
        Restart();
    }
    else
    {
        // Wrap around to 0x8000 if it reaches 0xFFFF
        if (++m_currentAddress == 0x0000)
            m_currentAddress = 0x8000;
    }

    return m_remainingSamples == 0;
}

inline void DMCChannel::LoadSampleAddress()
{
    // Sample address = %11AAAAAA.AA000000 = $C000 + (A * 64)
    m_currentAddress = 0xC000 + ((uint16_t)(m_register.sampleAddress) << 6);
}

inline void DMCChannel::LoadSampleLength()
{
    // Sample length = %LLLL.LLLL0001 = (L * 16) + 1 bytes
    m_remainingSamples = ((uint16_t)(m_register.sampleLength) << 4) + 1;
}

void DMCChannel::WriteData(uint16_t address, uint8_t data, const Mode& mode)
{
    if (address == 0x4010)
    {
        m_register.flags.reg = data;
        m_currentRate = (mode == Mode::NTSC ? NesEmulator::Cst::APU_DMC_RATE_NTSC
                                            : NesEmulator::Cst::APU_DMC_RATE_PAL)[m_register.flags.rate];
    }
    else if (address == 0x4011)
    {
        m_register.directLoad = data & 0x7F;
        m_currentOutput = m_register.directLoad;
    }
    else if (address == 0x4012)
    {
        m_register.sampleAddress = data;
        LoadSampleAddress();
    }
    else if (address == 0x4013)
    {
        m_register.sampleLength = data;
        LoadSampleLength();
    }
}

void DMCChannel::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    m_register.SerializeTo(visitor);
    visitor.WriteValue(m_currentRate);

    visitor.WriteValue(m_sampleBufferIsEmpty);
    visitor.WriteValue(m_currentAddress);
    visitor.WriteValue(m_remainingSamples);
    visitor.WriteValue(m_sampleBuffer);

    visitor.WriteValue(m_enabled);
    visitor.WriteValue(m_currentRate);
    visitor.WriteValue(m_currentOutput);
    visitor.WriteValue(m_bitShiftRegister);
    visitor.WriteValue(m_bitsRemaining);
    visitor.WriteValue(m_timer);
}

void DMCChannel::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    m_register.DeserializeFrom(visitor);
    visitor.ReadValue(m_currentRate);

    visitor.ReadValue(m_sampleBufferIsEmpty);
    visitor.ReadValue(m_currentAddress);
    visitor.ReadValue(m_remainingSamples);
    visitor.ReadValue(m_sampleBuffer);

    visitor.ReadValue(m_enabled);
    visitor.ReadValue(m_currentRate);
    visitor.ReadValue(m_currentOutput);
    visitor.ReadValue(m_bitShiftRegister);
    visitor.ReadValue(m_bitsRemaining);
    visitor.ReadValue(m_timer);
}