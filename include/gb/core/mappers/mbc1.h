#pragma once

#include <core/mappers/mapperBase.h>

namespace GBEmulator
{
class MBC1 : public MapperBase
{
public:
    MBC1(const Header& header);

    void SerializeTo(Common::Utils::IWriteVisitor& visitor) const override;
    void DeserializeFrom(Common::Utils::IReadVisitor& visitor) override;

    void Reset() override;

    bool WriteByte(uint16_t addr, uint8_t data) override;

private:
    bool m_advancedBankingMode = false;
};
} // namespace GBEmulator