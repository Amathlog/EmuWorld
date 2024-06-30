#include <core/audio/enveloppe.h>

using NesEmulator::Enveloppe;

void Enveloppe::Clock(bool loop)
{
    if (!start)
    {
        if (dividerCount == 0)
        {
            dividerCount = volume;

            if (decayCount == 0 && loop)
            {
                decayCount = 15;
            }
            else if (decayCount != 0)
            {
                decayCount--;
            }
        }
        else
        {
            dividerCount--;
        }
    }
    else
    {
        start = false;
        decayCount = 15;
        dividerCount = volume;
    }

    uint16_t temp;
    if (disable)
    {
        temp = volume;
    }
    else
    {
        temp = decayCount;
    }

    if (temp != output)
    {
        output = temp;
        updated = true;
    }
    else
    {
        updated = false;
    }
}

void Enveloppe::Reset()
{
    start = false;
    disable = false;
    dividerCount = 0;
    volume = 0;
    output = 0;
    decayCount = 0;
    updated = true;
}

void Enveloppe::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    visitor.WriteValue(start);
    visitor.WriteValue(disable);
    visitor.WriteValue(dividerCount);
    visitor.WriteValue(volume);
    visitor.WriteValue(output);
    visitor.WriteValue(decayCount);
}

void Enveloppe::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    visitor.ReadValue(start);
    visitor.ReadValue(disable);
    visitor.ReadValue(dividerCount);
    visitor.ReadValue(volume);
    visitor.ReadValue(output);
    visitor.ReadValue(decayCount);
}