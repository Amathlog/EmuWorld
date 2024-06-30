#pragma once

#include <core/mapper.h>
#include <cstdint>

namespace NesEmulator
{
class Mapper_002 : public IMapper
{
public:
    Mapper_002(const iNESHeader& header, Mapping& mapping);
    ~Mapper_002() = default;

    bool MapReadCPU(uint16_t address, uint32_t& mappedAddress, uint8_t& data) override;
    bool MapWriteCPU(uint16_t address, uint32_t& mappedAddress, uint8_t data) override;
    bool MapReadPPU(uint16_t address, uint32_t& mappedAddress, uint8_t& data) override;
    bool MapWritePPU(uint16_t address, uint32_t& mappedAddress, uint8_t data) override;
    void Reset() override
    {
        IMapper::Reset();
        InternalReset();
    };

    void SerializeTo(Common::Utils::IWriteVisitor& visitor) const override;
    void DeserializeFrom(Common::Utils::IReadVisitor& visitor) override;

private:
    void InternalReset();
    void UpdateMapping();
    uint8_t m_currentSwitchPrgBank = 0;
};
} // namespace NesEmulator