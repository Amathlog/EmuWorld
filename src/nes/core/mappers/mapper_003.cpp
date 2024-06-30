#include "core/mapper.h"
#include <cassert>
#include <core/mappers/mapper_003.h>
#include <cstdint>

using NesEmulator::Mapper_003;
using NesEmulator::Mapping;

Mapper_003::Mapper_003(const iNESHeader& header, Mapping& mapping)
    : IMapper(header, mapping)
{
    assert(m_nbPrgBanks > 0 && m_nbPrgBanks <= 2 && (m_nbChrBanks >= 2) &&
           "Wrong number of prgBanks or chrBans in mapper 003");

    InternalReset();
}

bool Mapper_003::MapReadCPU(uint16_t address, uint32_t& mappedAddress, uint8_t& /*data*/)
{
    if (address >= 0x6000 && address <= 0x7FFF)
    {
        // PrgRam range
        mappedAddress = address & 0x01FF;
        return true;
    }
    else if (address >= 0x8000 && (address <= 0xBFFF || m_nbPrgBanks == 1))
    {
        // Low bank address (or mirroring high bank if there is only one prgBank)
        mappedAddress = address & 0x3FFF;
        return true;
    }
    else if (address >= 0xC000 && address <= 0xFFFF)
    {
        // High bank address
        mappedAddress = address & 0x7FFF;
        return true;
    }

    return false;
}

bool Mapper_003::MapWriteCPU(uint16_t address, uint32_t& mappedAddress, uint8_t data)
{
    if (address >= 0x6000 && address <= 0x7FFF)
    {
        // PrgRam range
        mappedAddress = address & 0x01FF;
        return true;
    }
    if (address >= 0x8000 && address <= 0xFFFF)
    {
        m_currentSwitchChrBank = data;
        UpdateMapping();
    }
    return false;
}

bool Mapper_003::MapReadPPU(uint16_t address, uint32_t& mappedAddress, uint8_t& /*data*/)
{
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        mappedAddress = m_currentSwitchChrBank * 0x2000 + address;
        return true;
    }

    return false;
}

bool Mapper_003::MapWritePPU(uint16_t /*address*/, uint32_t& /*mappedAddress*/, uint8_t /*data*/) { return false; }

void Mapper_003::InternalReset()
{
    m_currentSwitchChrBank = 0;

    if (m_nbPrgBanks == 1)
        m_mapping.m_prgMapping = {0, 1, 0, 1};
    else
        m_mapping.m_prgMapping = {0, 1, 2, 3};

    m_mapping.m_prgRamMapping.fill(0);
    m_mapping.m_chrMapping = {0, 1, 2, 3, 4, 5, 6, 7};
}

void Mapper_003::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    IMapper::SerializeTo(visitor);

    visitor.WriteValue(m_currentSwitchChrBank);
}

void Mapper_003::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    IMapper::DeserializeFrom(visitor);

    visitor.ReadValue(m_currentSwitchChrBank);
    UpdateMapping();
}

inline void Mapper_003::UpdateMapping()
{
    uint16_t currentBank = m_currentSwitchChrBank * 8;
    m_mapping.m_chrMapping = {currentBank,
                              (uint16_t)(currentBank + 1),
                              (uint16_t)(currentBank + 2),
                              (uint16_t)(currentBank + 3),
                              (uint16_t)(currentBank + 4),
                              (uint16_t)(currentBank + 5),
                              (uint16_t)(currentBank + 6),
                              (uint16_t)(currentBank + 7)};
}