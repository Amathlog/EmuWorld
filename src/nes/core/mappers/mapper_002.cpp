#include "core/mapper.h"
#include <cassert>
#include <core/mappers/mapper_002.h>
#include <cstdint>

using NesEmulator::Mapper_002;

Mapper_002::Mapper_002(const iNESHeader& header, Mapping& mapping)
    : IMapper(header, mapping)
{
    assert(m_nbPrgBanks >= 2 && (m_nbChrBanks == 0 || m_nbChrBanks == 2) &&
           "Wrong number of prgBanks or chrBans in mapper 002");

    InternalReset();
}

bool Mapper_002::MapReadCPU(uint16_t address, uint32_t& mappedAddress, uint8_t& /*data*/)
{
    // Should not be called anymore

    if (address >= 0x6000 && address <= 0x7FFF)
    {
        // PrgRam range
        mappedAddress = address & 0x01FF;
        return true;
    }
    else if (address >= 0x8000 && address <= 0xBFFF)
    {
        // Low bank address
        mappedAddress = (m_currentSwitchPrgBank * 0x4000) + (address & 0x3FFF);
        return true;
    }
    else if (address >= 0xC000 && address <= 0xFFFF)
    {
        // High bank address, always mapped to the lastest bank on the cartridge
        mappedAddress = (m_nbPrgBanks - 1) * 0x4000 + (address & 0x3FFF);
        return true;
    }

    return false;
}

bool Mapper_002::MapWriteCPU(uint16_t address, uint32_t& mappedAddress, uint8_t data)
{
    if (address >= 0x6000 && address <= 0x7FFF)
    {
        // PrgRam range
        mappedAddress = address & 0x01FF;
        return true;
    }
    else if (address >= 0x8000 && address <= 0xFFFF)
    {
        m_currentSwitchPrgBank = (data & 0x0F);
        UpdateMapping();
    }
    return false;
}

bool Mapper_002::MapReadPPU(uint16_t address, uint32_t& mappedAddress, uint8_t& /*data*/)
{
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        mappedAddress = address;
        return true;
    }

    return false;
}

bool Mapper_002::MapWritePPU(uint16_t address, uint32_t& mappedAddress, uint8_t /*data*/)
{
    if (m_nbChrBanks == 0 && address >= 0x0000 && address <= 0x1FFF)
    {
        mappedAddress = address;
        return true;
    }

    return false;
}

void Mapper_002::InternalReset()
{
    m_currentSwitchPrgBank = 0;

    uint16_t lastBank = 2 * (m_nbPrgBanks - 1);
    m_mapping.m_prgMapping = {0, 1, lastBank, (uint16_t)(lastBank + 1)};

    m_mapping.m_prgRamMapping.fill(0);
    m_mapping.m_chrMapping = {0, 1, 2, 3, 4, 5, 6, 7};
}

void Mapper_002::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    IMapper::SerializeTo(visitor);

    visitor.WriteValue(m_currentSwitchPrgBank);
}

void Mapper_002::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    IMapper::DeserializeFrom(visitor);

    visitor.ReadValue(m_currentSwitchPrgBank);
    UpdateMapping();
}

inline void Mapper_002::UpdateMapping()
{
    m_mapping.m_prgMapping[0] = 2 * m_currentSwitchPrgBank;
    m_mapping.m_prgMapping[1] = 2 * m_currentSwitchPrgBank + 1;
}