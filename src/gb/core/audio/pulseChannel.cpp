#include <core/audio/pulseChannel.h>

#include <core/apu.h>
#include <core/constants.h>

using GBEmulator::APU;
using GBEmulator::PulseChannel;
using GBEmulator::PulseOscillator;

PulseOscillator::PulseOscillator(double sampleRate)
    : m_sampleRate(sampleRate)
{
}

double PulseOscillator::Tick()
{
    if (m_duty == 0.0 || m_freq == 0.0)
        return 0.0;

    m_phase += m_freq / m_sampleRate;

    while (m_phase > 1.0)
        m_phase -= 1.0f;

    while (m_phase < 0.0)
        m_phase += 1.0;

    return m_phase <= m_duty ? -1.0 : 1.0;
}

PulseChannel::PulseChannel(int number)
    : m_number(number)
    , m_oscillator(GBEmulator::APU_SAMPLE_RATE_D)
{
}

void PulseChannel::Update(uint8_t divCounter)
{
    // Should be called every 1/512 seconds
    bool clock256Hz = divCounter % 2 == 0;
    bool clock128Hz = divCounter % 4 == 2;
    bool clock64Hz = divCounter % 8 == 7;

    // Update every n / 256 seconds
    if (m_freqMsbReg.lengthEnable && m_lengthCounter != 0 && clock256Hz && --m_lengthCounter == 0)
    {
        SetEnable(false);
    }

    // Update every n / 128 seconds
    if (m_enabled && m_sweepReg.time != 0 && clock128Hz && --m_sweepCounter == 0)
    {
        Sweep();
    }

    // Enveloppe updated every n / 64 seconds
    if (m_volumeCounter > 0 && clock64Hz && --m_volumeCounter == 0)
    {
        if (m_volumeReg.enveloppeDirection == 0 && m_volume > 0)
        {
            // Decrease
            --m_volume;
        }
        else if (m_volumeReg.enveloppeDirection == 1 && m_volume < 0x0F)
        {
            // Increase
            ++m_volume;
        }

        // Reload only if we are still in the range
        if (m_volume > 0 && m_volume < 0x0F)
            m_volumeCounter = m_volumeReg.nbEnveloppeSweep == 0 ? 8 : m_volumeReg.nbEnveloppeSweep;
    }

    if (m_frequencyChanged)
    {
        m_oscillator.SetFrequency(m_frequency);
        m_frequencyChanged = false;
    }

    if (m_dutyChanged)
    {
        float newDuty = 0.125f;
        if (m_waveReg.duty == 0x01)
        {
            newDuty = 0.25f;
        }
        else if (m_waveReg.duty == 0x02)
        {
            newDuty = 0.5f;
        }
        else if (m_waveReg.duty == 0x03)
        {
            newDuty = 0.75f;
        }

        m_oscillator.SetDuty(newDuty);
        m_dutyChanged = false;
    }
}

void PulseChannel::Reset()
{
    m_sweepReg.reg = 0x00;
    m_waveReg.reg = 0x00;
    m_volumeReg.reg = 0x00;
    m_freqLsb = 0x00;
    m_freqMsbReg.reg = 0x00;
    m_lengthCounter = 0;
    m_sweepCounter = 0;
    m_volumeCounter = 0;
    m_enabled = false;
    m_frequency = 0.0;
    m_combinedFreq = 0x0000;
    m_volume = 0;

    m_frequencyChanged = false;
    m_dutyChanged = false;

    m_oscillator.Reset();
}

void PulseChannel::WriteByte(uint16_t addr, uint8_t data, const APU* apu)
{
    switch (addr)
    {
    case 0x00:
        // Sweep (only channel 1)
        if (m_number == 1)
        {
            m_sweepReg.reg = data;
            Sweep();
        }
        break;
    case 0x01:
        // Wave
        m_waveReg.reg = data;
        m_lengthCounter = 64 - m_waveReg.length;
        m_dutyChanged = true;
        break;
    case 0x02:
        // Enveloppe
        m_volumeReg.reg = data;
        m_volumeCounter = m_volumeReg.nbEnveloppeSweep == 0 ? 8 : m_volumeReg.nbEnveloppeSweep;
        m_volume = m_volumeReg.initialVolume;
        // If DAC is off, disable the channel
        if (!IsDACOn())
        {
            SetEnable(false);
        }
        break;
    case 0x3:
        // Freq lsb
        m_freqLsb = data;
        m_combinedFreq = ((uint16_t)m_freqMsbReg.freqMsb << 8) | m_freqLsb;
        UpdateFreq();
        break;
    case 0x04:
    {
        // Freq Msb
        const bool lengthWasDisabled = !m_freqMsbReg.lengthEnable;
        m_freqMsbReg.reg = data;
        m_combinedFreq = ((uint16_t)m_freqMsbReg.freqMsb << 8) | m_freqLsb;
        if (!!m_freqMsbReg.initial)
            Restart();
        UpdateFreq();

        // Obscure behavior: If the length is not clocked at next APU clock (updatecall & 0x01 != 0)
        // and the length counter was disabled and is now enabled and the length counter is not 0,
        // clock the length.
        if (lengthWasDisabled && !!(data & 0x40) && m_lengthCounter != 0 && !!(apu->GetDivCounter() & 0x01))
        {
            if (--m_lengthCounter == 0)
            {
                // Weird thing, if the trigger is on and we clock the length and it reaches 0,
                // don't disable and reload the length counter with 63
                if (!!m_freqMsbReg.initial)
                    m_lengthCounter = 0x3F;
                else
                    SetEnable(false);
            }
        }

        // Trigger is fire and forget, so set back the bit to 0.
        m_freqMsbReg.initial = 0;

        // Reset the length enabled if it was changed by the Restart
        m_freqMsbReg.lengthEnable = !!(data & 0x40);

        break;
    }
    default:
        break;
    }
}

uint8_t PulseChannel::ReadByte(uint16_t addr) const
{
    switch (addr)
    {
    case 0x00:
        // Sweep
        return m_number == 1 ? (m_sweepReg.reg | 0x80) : 0xFF;
    case 0x01:
        // Wave
        return m_waveReg.reg | 0x3F;
    case 0x02:
        // Enveloppe
        return m_volumeReg.reg;
    case 0x03:
        // Freq lsb
        return 0xFF;
    case 0x04:
    {
        // Freq Msb
        return m_freqMsbReg.reg | 0xBF;
    }
    default:
        break;
    }

    return 0x00;
}

double PulseChannel::GetSample()
{
    const double currentVolume = (double)m_volume / 0x0F;
    const double currentSample = m_oscillator.Tick();
    return m_enabled ? currentVolume * currentSample : 0.0;
}

void PulseChannel::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    visitor.WriteValue(m_sweepReg);
    visitor.WriteValue(m_waveReg);
    visitor.WriteValue(m_volumeReg);
    visitor.WriteValue(m_freqLsb);
    visitor.WriteValue(m_freqMsbReg);
    visitor.WriteValue(m_lengthCounter);
    visitor.WriteValue(m_sweepCounter);
    visitor.WriteValue(m_volumeCounter);
    visitor.WriteValue(m_enabled);

    visitor.WriteValue(m_frequency);
    visitor.WriteValue(m_combinedFreq);
    visitor.WriteValue(m_frequencyChanged);
    visitor.WriteValue(m_dutyChanged);
    visitor.WriteValue(m_volume);
}

void PulseChannel::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    visitor.ReadValue(m_sweepReg);
    visitor.ReadValue(m_waveReg);
    visitor.ReadValue(m_volumeReg);
    visitor.ReadValue(m_freqLsb);
    visitor.ReadValue(m_freqMsbReg);
    visitor.ReadValue(m_lengthCounter);
    visitor.ReadValue(m_sweepCounter);
    visitor.ReadValue(m_volumeCounter);
    visitor.ReadValue(m_enabled);

    visitor.ReadValue(m_frequency);
    visitor.ReadValue(m_combinedFreq);
    visitor.ReadValue(m_frequencyChanged);
    visitor.ReadValue(m_dutyChanged);
    visitor.ReadValue(m_volume);
}

void PulseChannel::UpdateFreq()
{
    CheckOverflow(m_combinedFreq);

    if (m_combinedFreq == 0)
        return;

    m_frequency = 131072.0 / (2048.0 - m_combinedFreq);
    m_frequencyChanged = true;

    m_freqLsb = m_combinedFreq & 0x00FF;
    m_freqMsbReg.freqMsb = (m_combinedFreq & 0x0700) >> 8;
}

void PulseChannel::Restart()
{
    const bool wasEnabled = m_enabled;

    SetEnable(true);
    if (m_lengthCounter == 0)
    {
        m_lengthCounter = 64;
        m_freqMsbReg.lengthEnable = 0;
    }

    m_volumeCounter = m_volumeReg.nbEnveloppeSweep;
    m_sweepCounter = m_sweepReg.time;
    m_volume = m_volumeReg.initialVolume;

    // If DAC is off, re-disable the channel
    if (!IsDACOn())
    {
        SetEnable(false);
    }
}

void PulseChannel::CheckOverflow(uint16_t newFreq)
{
    if (newFreq >= 2047)
    {
        SetEnable(false);
        m_combinedFreq = 0;
    }
}

void PulseChannel::Sweep()
{
    if (m_sweepReg.time == 0)
        return;

    int16_t frqChange = m_combinedFreq >> m_sweepReg.shift;
    m_combinedFreq += m_sweepReg.decrease ? -frqChange : frqChange;
    m_sweepCounter = m_sweepReg.time;
    UpdateFreq();

    // Then re-do it once and check overflow only
    frqChange = m_combinedFreq >> m_sweepReg.shift;
    uint16_t newFreq = m_combinedFreq + (m_sweepReg.decrease ? -frqChange : frqChange);
    CheckOverflow(newFreq);
}