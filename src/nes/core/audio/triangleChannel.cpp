#include <core/audio/triangleChannel.h>
#include <core/constants.h>

using NesEmulator::TriangleChannel;
using NesEmulator::TriangleOscillator;
using NesEmulator::TriangleRegister;

////////////////////////////////////////////////////////////////////////
// TRIANGLE REGISTER
////////////////////////////////////////////////////////////////////////
void TriangleRegister::Reset()
{
    control = 0;
    linearCounterLoad = 0;
    timer = 0;
    lengthCounterLoad = 0;
}

void TriangleRegister::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    visitor.WriteValue(control);
    visitor.WriteValue(linearCounterLoad);
    visitor.WriteValue(timer);
    visitor.WriteValue(lengthCounterLoad);
}

void TriangleRegister::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    visitor.ReadValue(control);
    visitor.ReadValue(linearCounterLoad);
    visitor.ReadValue(timer);
    visitor.ReadValue(lengthCounterLoad);
}

////////////////////////////////////////////////////////////////////////
// TRIANGLE OSCILLATOR
////////////////////////////////////////////////////////////////////////
double TriangleOscillator::Tick()
{
    m_phase += m_phaseIncrement;

    while (m_phase > 1.0)
        m_phase -= 1.0;

    while (m_phase < 0.0)
        m_phase += 1.0;

    double ret;
    if (m_phase <= 0.5)
        ret = m_phase * 2.0;
    else
        ret = (1.0 - m_phase) * 2.0;

    return (ret * 2.0) - 1.0;
}

void TriangleOscillator::SetFrequency(double freq)
{
    m_freq = freq;
    m_phaseIncrement = freq / NesEmulator::Cst::SAMPLE_RATE;
}

////////////////////////////////////////////////////////////////////////
// TRIANGLE CHANNEL
////////////////////////////////////////////////////////////////////////

void TriangleChannel::Reset()
{
    m_register.Reset();

    m_frequency = 0.0;
    m_enableValue = 0.0;
    m_linearCounter = 0;
    m_lengthCounter = 0;
    m_linearControlFlag = 0;

    m_oscillator.Reset();
}

void TriangleChannel::ClockLinear(bool isEnabled)
{
    if (!isEnabled)
    {
        m_linearCounter = 0;
        return;
    }

    if (m_linearControlFlag > 0)
    {
        m_linearCounter = m_register.linearCounterLoad;
    }
    else if (m_linearCounter > 0)
    {
        m_linearCounter--;
    }

    if (m_register.control == 0)
        SetLinearControlFlag(0);
}

void TriangleChannel::ClockLength(bool isEnabled)
{
    if (!isEnabled)
    {
        m_lengthCounter = 0;
    }
    else if (m_lengthCounter > 0)
    {
        m_lengthCounter--;
    }
}

void TriangleChannel::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    m_register.SerializeTo(visitor);

    visitor.WriteValue(m_frequency);
    visitor.WriteValue(m_lengthCounter);
    visitor.WriteValue(m_linearCounter);
    visitor.WriteValue(m_linearControlFlag);
}

void TriangleChannel::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    m_register.DeserializeFrom(visitor);

    visitor.ReadValue(m_frequency);
    visitor.ReadValue(m_lengthCounter);
    visitor.ReadValue(m_linearCounter);
    visitor.ReadValue(m_linearControlFlag);
}

void TriangleChannel::Update(double cpuFrequency)
{
    double newFrequency = cpuFrequency / (32.0 * (double)(m_register.timer + 1));
    double newEnableValue = m_linearCounter > 0 && m_lengthCounter > 0;
    if (newFrequency != m_frequency || newEnableValue != m_enableValue)
    {
        m_frequency = newFrequency;
        m_enableValue = newEnableValue;
        m_oscillator.SetFrequency(newFrequency);
    }
}

double TriangleChannel::GetSample() { return m_enableValue != 0.0 ? m_oscillator.Tick() : 0.0; }

void TriangleChannel::ReloadCounter() { m_lengthCounter = Cst::APU_LENGTH_TABLE[m_register.lengthCounterLoad]; }