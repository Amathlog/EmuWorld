#pragma once

#include <common/utils/visitor.h>

namespace NesEmulator
{
namespace Utils
{
class IWriteVisitor;
class IReadVisitor;
} // namespace Utils

class ISerializable
{
public:
    virtual ~ISerializable() = default;
    virtual void SerializeTo(Common::Utils::IWriteVisitor& visitor) const = 0;
    virtual void DeserializeFrom(Common::Utils::IReadVisitor& visitor) = 0;
};
} // namespace NesEmulator