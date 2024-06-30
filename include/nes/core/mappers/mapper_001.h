#pragma once

#include <common/utils/visitor.h>
#include <core/mapper.h>
#include <cstdint>
#include <vector>

namespace NesEmulator
{
class Mapper_001 : public IMapper
{
public:
    Mapper_001(const iNESHeader& header, Mapping& mapping);
    ~Mapper_001() = default;

    bool MapReadCPU(uint16_t address, uint32_t& mappedAddress, uint8_t& data) override;
    bool MapWriteCPU(uint16_t address, uint32_t& mappedAddress, uint8_t data) override;
    bool MapReadPPU(uint16_t address, uint32_t& mappedAddress, uint8_t& data) override;
    bool MapWritePPU(uint16_t address, uint32_t& mappedAddress, uint8_t data) override;

    void Reset() override { InternalReset(); };

    void SerializeTo(Common::Utils::IWriteVisitor& visitor) const override;
    void DeserializeFrom(Common::Utils::IReadVisitor& visitor) override;

private:
    void InternalReset();
    void ResetShiftRegister();
    void ResetControlRegister();
    void HandleLoadRegister(uint8_t data);

    void UpdateMapping();

    uint8_t m_shiftRegister = 0x00;
    uint8_t m_shiftCounter = 0x00;
    uint8_t m_internalRegister = 0x00;
    bool m_loadRegisterDone = false;

    uint8_t m_currentPrgBankSwitch = 0;
    bool m_PrgBank0IsSwitch = false;
    uint8_t m_currentChrBank0 = 0;
    uint8_t m_currentChrBank1 = 1;
    bool m_32kBModePrgBank = false;
    bool m_8kBModeChrBank = false;
};
} // namespace NesEmulator