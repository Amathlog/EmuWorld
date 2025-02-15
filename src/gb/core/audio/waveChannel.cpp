#include <core/audio/waveChannel.h>
#include <core/constants.h>

using GBEmulator::WaveChannel;
using GBEmulator::WaveOscillator;

WaveOscillator::WaveOscillator() { m_realSampleDuration = 1.0 / GBEmulator::APU_SAMPLE_RATE_D; }

void WaveOscillator::Reset()
{
    m_ptr = 0;
    m_sampleDuration = 0.0;
    m_volume = 0.0;
    m_elaspedTime = 0.0;
}

void WaveOscillator::SetFrequency(double freq)
{
    if (freq == 0.0)
    {
        m_sampleDuration = 0.0;
        m_volume = 0.0;
    }
    else
    {
        m_sampleDuration = 1.0 / (freq * 32.0);
    }
}

double WaveOscillator::GetSample()
{
    if (m_volume == 0.0)
    {
        return 0.0;
    }

    double value = m_volume * (double)m_data[m_ptr] / 0x0F;
    m_elaspedTime += m_realSampleDuration;
    if (m_elaspedTime >= m_sampleDuration)
    {
        // Equivalent to m_ptr = (m_ptr + 1) % 32
        m_ptr = (m_ptr + 1) & 0x1F;
        m_elaspedTime -= m_sampleDuration;
    }

    return value;
}

double WaveChannel::GetSample()
{
    const double currentSample = m_oscillator.GetSample();
    return m_enabled ? currentSample : 0.0;
}

void WaveChannel::Update()
{
    // Should be called every 1/512 seconds
    bool clock256Hz = (m_nbUpdateCalls & 0x1) == 0;

    // Update every n / 256 seconds
    if (m_freqMsbReg.lengthEnable && m_lengthCounter != 0 && clock256Hz && --m_lengthCounter == 0)
    {
        m_enabled = false;
    }

    m_nbUpdateCalls++;
}

void WaveChannel::Reset()
{
    m_soundLength = 0x00;
    m_volumeReg.reg = 0x00;
    m_freqMsbReg.reg = 0x00;
    m_lengthCounter = 0;
    m_enabled = false;
    m_DAC = false;
    m_freq = 0x0000;
    m_oscillator.Reset();

    m_nbUpdateCalls = 0;
}

void WaveChannel::WriteByte(uint16_t addr, uint8_t data)
{
    switch (addr)
    {
    case 0xFF1A:
        // Enable DAC
        m_DAC = (data & 0x80) > 0;
        if (!m_DAC)
            m_enabled = false;
        break;
    case 0xFF1B:
        // Sound length
        m_soundLength = data;
        m_lengthCounter = 256 - m_soundLength;
        break;
    case 0xFF1C:
    {
        // Volume
        m_volumeReg.reg = data;
        SetVolume();
        break;
    }
    case 0xFF1D:
        // Freq LSB
        m_freq = (m_freq & 0x0700) | data;
        SetFrequency();
        break;
    case 0xFF1E:
        // Freq MSB
        m_freqMsbReg.reg = data;
        m_freq = ((uint16_t)m_freqMsbReg.freqMsb << 8) | (m_freq & 0x00FF);
        if (m_freqMsbReg.initial > 0)
            Restart();
        // Trigger is fire and forget, so set back the bit to 0.
        m_freqMsbReg.initial = 0;
        break;
    default:
        break;
    }

    if (addr >= 0xFF30 && addr <= 0xFF3F && !m_enabled)
    {
        // Wave Table. Only accessible if channel disabled
        uint8_t position = (addr - 0xFF30) << 1;
        uint8_t firstSample = (data & 0xF0) >> 4;
        uint8_t secondSample = data & 0x0F;

        m_oscillator.m_data[position] = firstSample;
        m_oscillator.m_data[position + 1] = secondSample;
    }
}

uint8_t WaveChannel::ReadByte(uint16_t addr) const
{
    switch (addr)
    {
    case 0xFF1A:
        return m_DAC ? 0xFF : 0x7F;
    case 0xFF1B:
        return 0xFF;
    case 0xFF1C:
        return m_volumeReg.reg | 0x9F;
    case 0xFF1D:
        return 0xFF;
    case 0xFF1E:
        return m_freqMsbReg.reg | 0xBF;
    }

    if (addr >= 0xFF30 && addr <= 0xFF3F && !m_enabled)
    {
        // Wave Table. Only accessible if channel disabled
        uint8_t position = (addr - 0xFF30) * 2;
        uint8_t firstSample = m_oscillator.m_data[position];
        uint8_t secondSample = m_oscillator.m_data[position + 1];
        return ((firstSample & 0x0F) << 4) | (secondSample & 0x0F);
    }

    return 0xFF;
}

void WaveChannel::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    visitor.WriteValue(m_volumeReg);
    visitor.WriteValue(m_soundLength);
    visitor.WriteValue(m_lengthCounter);
    visitor.WriteValue(m_freqMsbReg);
    visitor.WriteValue(m_freq);
    visitor.WriteValue(m_enabled);
    visitor.WriteValue(m_DAC);
    visitor.WriteValue(m_nbUpdateCalls);
}

void WaveChannel::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    visitor.ReadValue(m_volumeReg);
    visitor.ReadValue(m_soundLength);
    visitor.ReadValue(m_lengthCounter);
    visitor.ReadValue(m_freqMsbReg);
    visitor.ReadValue(m_freq);
    visitor.ReadValue(m_enabled);
    visitor.ReadValue(m_DAC);
    visitor.ReadValue(m_nbUpdateCalls);
}

void WaveChannel::SetFrequency()
{
    const double newFreq = 65536.0 / (2048 - m_freq);
    m_oscillator.SetFrequency(newFreq);
}

void WaveChannel::SetVolume()
{
    float newVolume = 0.f;
    if (m_enabled)
    {
        switch (m_volumeReg.volume)
        {
        case 1:
            newVolume = 1.f;
            break;
        case 2:
            newVolume = 0.5f;
            break;
        case 3:
            newVolume = 0.25f;
            break;
        default:
            newVolume = 0.0f;
            break;
        }
    }

    m_oscillator.m_volume = newVolume;
}

void WaveChannel::Restart()
{
    m_enabled = true;
    m_oscillator.Reset();
    if (m_lengthCounter == 0)
        m_lengthCounter = 256;

    SetFrequency();
    SetVolume();

    if (!m_DAC)
        m_enabled = false;
}