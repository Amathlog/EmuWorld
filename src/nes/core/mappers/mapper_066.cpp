#include "core/mapper.h"
#include <cassert>
#include <core/mappers/mapper_066.h>
#include <cstdint>

using NesEmulator::Mapper_066;
using NesEmulator::Mapping;

Mapper_066::Mapper_066(const iNESHeader& header, Mapping& mapping)
    : IMapper(header, mapping)
{
    assert(m_nbPrgBanks >= 2 && m_nbChrBanks >= 2 && "Wrong number of prgBanks or chrBans in mapper 066");

    InternalReset();
}

bool Mapper_066::MapReadCPU(uint16_t address, uint32_t& mappedAddress, uint8_t& /*data*/)
{
    if (address >= 0x6000 && address <= 0x7FFF)
    {
        // PrgRam range
        mappedAddress = address & 0x01FF;
        return true;
    }
    else if (address >= 0x8000 && address <= 0xFFFF)
    {
        // Low bank address
        mappedAddress = (m_currentSwitchPrgBank * 0x8000) + (address & 0x7FFF);
        return true;
    }

    return false;
}

bool Mapper_066::MapWriteCPU(uint16_t address, uint32_t& mappedAddress, uint8_t data)
{
    if (address >= 0x6000 && address <= 0x7FFF)
    {
        // PrgRam range
        mappedAddress = address & 0x01FF;
        return true;
    }
    else if (address >= 0x8000 && address <= 0xFFFF)
    {
        m_currentSwitchChrBank = (data & 0x03);
        m_currentSwitchPrgBank = ((data >> 4) & 0x03);
        UpdateMapping();
    }
    return false;
}

bool Mapper_066::MapReadPPU(uint16_t address, uint32_t& mappedAddress, uint8_t& /*data*/)
{
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        mappedAddress = m_currentSwitchChrBank * 0x2000 + address;
        return true;
    }

    return false;
}

bool Mapper_066::MapWritePPU(uint16_t /*address*/, uint32_t& /*mappedAddress*/, uint8_t /*data*/)
{
    // if (m_nbChrBanks == 0 && address >= 0x0000 && address <= 0x1FFF)
    // {
    //     mappedAddress = address;
    //     return true;
    // }

    return false;
}

void Mapper_066::InternalReset()
{
    m_currentSwitchPrgBank = 0;
    m_currentSwitchChrBank = 0;

    m_mapping.m_prgMapping = {0, 1, 2, 3};
    m_mapping.m_chrMapping = {0, 1, 2, 3, 4, 5, 6, 7};
    m_mapping.m_prgRamMapping.fill(0);
}

void Mapper_066::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    IMapper::SerializeTo(visitor);

    visitor.WriteValue(m_currentSwitchPrgBank);
    visitor.WriteValue(m_currentSwitchChrBank);
}

void Mapper_066::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    IMapper::DeserializeFrom(visitor);

    visitor.ReadValue(m_currentSwitchPrgBank);
    visitor.ReadValue(m_currentSwitchChrBank);

    UpdateMapping();
}

inline void Mapper_066::UpdateMapping()
{
    uint16_t currentPrgBank = 4 * m_currentSwitchPrgBank;
    m_mapping.m_prgMapping[0] = currentPrgBank;
    m_mapping.m_prgMapping[1] = currentPrgBank + 1;
    m_mapping.m_prgMapping[2] = currentPrgBank + 2;
    m_mapping.m_prgMapping[3] = currentPrgBank + 3;

    uint16_t currentChrBank = 8 * m_currentSwitchChrBank;
    m_mapping.m_chrMapping[0] = currentChrBank;
    m_mapping.m_chrMapping[1] = currentChrBank + 1;
    m_mapping.m_chrMapping[2] = currentChrBank + 2;
    m_mapping.m_chrMapping[3] = currentChrBank + 3;
    m_mapping.m_chrMapping[4] = currentChrBank + 4;
    m_mapping.m_chrMapping[5] = currentChrBank + 5;
    m_mapping.m_chrMapping[6] = currentChrBank + 6;
    m_mapping.m_chrMapping[7] = currentChrBank + 7;
}