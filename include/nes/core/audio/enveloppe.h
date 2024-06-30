#pragma once

#include <common/utils/visitor.h>
#include <cstdint>

namespace NesEmulator
{
struct Enveloppe
{
    void Clock(bool loop);

    void Reset();
    void SerializeTo(Common::Utils::IWriteVisitor& visitor) const;
    void DeserializeFrom(Common::Utils::IReadVisitor& visitor);

    bool start;
    bool disable;
    uint16_t dividerCount;
    uint16_t volume;
    uint16_t output;
    uint16_t decayCount;
    bool updated;
};
} // namespace NesEmulator