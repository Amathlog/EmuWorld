#include <core/audio/pulseChannel.h>
#include <core/constants.h>
#include <cstdint>
#include <string>

using NesEmulator::PulseChannel;
using NesEmulator::PulseOscillator;
using NesEmulator::PulseRegister;
using NesEmulator::Sweep;

//////////////////////////////////////////////////////////////
// PULSE REGISTER
//////////////////////////////////////////////////////////////
void PulseRegister::Reset()
{
    duty = 0;
    enveloppeLoop = 0;
    timer = 0;
    lengthCounterReload = 0;
}

void PulseRegister::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    visitor.WriteValue(duty);
    visitor.WriteValue(enveloppeLoop);
    visitor.WriteValue(timer);
    visitor.WriteValue(lengthCounterReload);
}

void PulseRegister::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    visitor.ReadValue(duty);
    visitor.ReadValue(enveloppeLoop);
    visitor.ReadValue(timer);
    visitor.ReadValue(lengthCounterReload);
}

//////////////////////////////////////////////////////////////
// PULSE OSCILLATOR
//////////////////////////////////////////////////////////////
double PulseOscillator::Tick()
{
    if (m_duty == 0.0 || m_freq == 0.0)
        return 0.0;

    m_phase += m_phaseIncrement;

    while (m_phase > 1.0)
        m_phase -= 1.0f;

    while (m_phase < 0.0)
        m_phase += 1.0;

    return m_phase <= m_duty ? -1.0 : 1.0;
}

void PulseOscillator::SetFrequency(double freq)
{
    m_freq = freq;
    m_phaseIncrement = m_freq / NesEmulator::Cst::SAMPLE_RATE;
}

//////////////////////////////////////////////////////////////
// SWEEP
//////////////////////////////////////////////////////////////
void Sweep::Reset()
{
    enabled = false;
    ;
    down = false;
    reload = false;
    shift = 0;
    timer = 0;
    period = 0;
    change = 0;
    mute = false;
    ;
}

void Sweep::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    visitor.WriteValue(enabled);
    visitor.WriteValue(down);
    visitor.WriteValue(reload);
    visitor.WriteValue(shift);
    visitor.WriteValue(timer);
    visitor.WriteValue(period);
    visitor.WriteValue(change);
    visitor.WriteValue(mute);
}

void Sweep::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    visitor.ReadValue(enabled);
    visitor.ReadValue(down);
    visitor.ReadValue(reload);
    visitor.ReadValue(shift);
    visitor.ReadValue(timer);
    visitor.ReadValue(period);
    visitor.ReadValue(change);
    visitor.ReadValue(mute);
}

void Sweep::Track(uint16_t target)
{
    // if (enabled)
    // {
    //     change = target >> shift;
    //     mute = (target < 8) || (target > 0x7FF);
    // }
    change = target >> shift;
    mute = (target < 8) | (target > 0x7FF);
}

void Sweep::Clock(uint16_t& target, bool isChannel1)
{
    if (timer == 0 && enabled && shift > 0 && !mute)
    {
        if (target >= 8 && change < 0x07FF)
        {
            int16_t temp = down ? -change : change;
            if (isChannel1 && down)
                temp--;

            target += temp;
        }
    }

    if (timer == 0 || reload)
    {
        timer = period;
        reload = false;
    }
    else
    {
        timer--;
    }

    mute = (target < 8) || (target > 0x7FF);
}

//////////////////////////////////////////////////////////////
// PULSE CHANNEL
//////////////////////////////////////////////////////////////

PulseChannel::PulseChannel(int number)
    : m_number(number)
    , m_frequency(440.0f)
{
}

void PulseChannel::Update(double cpuFrequency)
{
    double newEnableValue = m_lengthCounter > 0 ? 1.0 : 0.0;
    if (m_sweep.mute || m_register.timer <= 8)
        newEnableValue = 0.0;

    double newFrequency = m_register.timer > 8 ? (cpuFrequency / (16.0 * (double)(m_register.timer + 1))) : m_frequency;
    double newDutyCycle = 0.0;
    switch (m_register.duty)
    {
    case 0:
        newDutyCycle = 0.125;
        break;
    case 1:
        newDutyCycle = 0.25;
        break;
    case 2:
        newDutyCycle = 0.5;
        break;
    case 3:
        newDutyCycle = 0.75;
        break;
    }

    // Enveloppe output
    if (m_enveloppe.updated)
    {
        float envOutput = (float)(m_enveloppe.output) / 15.0f;
        m_enveloppe.updated = false;
    }

    if (newFrequency != m_frequency)
    {
        m_frequency = newFrequency;
        m_oscillator.SetFrequency(m_frequency);
    }

    if (newDutyCycle != m_dutyCycle)
    {
        m_dutyCycle = newDutyCycle;
        m_oscillator.SetDuty(newDutyCycle);
    }

    if (newEnableValue != m_enableValue)
    {
        m_enableValue = newEnableValue;
    }
}

void PulseChannel::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    m_register.SerializeTo(visitor);
    m_sweep.SerializeTo(visitor);
    m_enveloppe.SerializeTo(visitor);

    visitor.WriteValue(m_frequency);
    visitor.WriteValue(m_dutyCycle);
    visitor.WriteValue(m_enableValue);
    visitor.WriteValue(m_lengthCounter);
}

void PulseChannel::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    m_register.DeserializeFrom(visitor);
    m_sweep.DeserializeFrom(visitor);
    m_enveloppe.DeserializeFrom(visitor);

    visitor.ReadValue(m_frequency);
    visitor.ReadValue(m_dutyCycle);
    visitor.ReadValue(m_enableValue);
    visitor.ReadValue(m_lengthCounter);
}

double PulseChannel::GetSample()
{
    double currentVolume = (double)(m_enveloppe.output) / 15.0;
    return m_enableValue != 0.0 ? currentVolume * m_oscillator.Tick() : 0.0;
}

void PulseChannel::Clock(bool isEnabled)
{
    // Update length counter
    // Enveloppe loop also count as pulseHalt
    bool halt = m_register.enveloppeLoop > 0;

    if (!isEnabled)
    {
        m_lengthCounter = 0;
    }
    else if (m_lengthCounter > 0 && !halt)
    {
        m_lengthCounter--;
    }

    // Update sweep
    m_sweep.Clock(m_register.timer, m_number == 1);
}

void PulseChannel::ClockEnveloppe() { m_enveloppe.Clock(m_register.enveloppeLoop); }

void PulseChannel::Track() { m_sweep.Track(m_register.timer); }

void PulseChannel::Reset()
{
    m_register.Reset();
    m_sweep.Reset();
    m_enveloppe.Reset();

    m_frequency = 440.0f;
    m_dutyCycle = 0.0;
    m_enableValue = 0.0;
    m_lengthCounter = 0;

    m_oscillator.Reset();
}

void PulseChannel::ReloadCounter() { m_lengthCounter = Cst::APU_LENGTH_TABLE[m_register.lengthCounterReload]; }