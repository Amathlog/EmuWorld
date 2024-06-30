#include "core/mapper.h"
#include <cassert>
#include <core/mappers/mapper_040.h>
#include <cstdint>

using NesEmulator::Mapper_040;
using NesEmulator::Mapping;

Mapper_040::Mapper_040(const iNESHeader& header, Mapping& mapping)
    : IMapper(header, mapping)
{
    InternalReset();
}

bool Mapper_040::MapReadCPU(uint16_t address, uint32_t& mappedAddress, uint8_t& data)
{
    if (address >= 0x6000 && address <= 0x7FFF)
    {
        // "RAM" is bank 6
        mappedAddress = 6 * 0x2000 + (address & 0x1FFF);
        return true;
    }
    else if (address >= 0x8000 && address <= 0x9FFF)
    {
        // Bank 4
        mappedAddress = 4 * 0x2000 + (address & 0x1FFF);
        return true;
    }
    else if (address >= 0xA000 && address <= 0xBFFF)
    {
        // Bank 5
        mappedAddress = 5 * 0x2000 + (address & 0x1FFF);
        return true;
    }
    else if (address >= 0xC000 && address <= 0xDFFF)
    {
        // Switchable bank
        mappedAddress = m_switchableBank * 0x2000 + (address & 0x1FFF);
        return true;
    }
    else if (address >= 0xE000 && address <= 0xFFFF)
    {
        // Bank 7 (last one)
        mappedAddress = 7 * 0x2000 + (address & 0x1FFF);
        return true;
    }

    return false;
}

bool Mapper_040::MapWriteCPU(uint16_t address, uint32_t& mappedAddress, uint8_t data)
{
    if (address >= 0x6000 && address <= 0x7FFF)
    {
        // Nothing here
    }
    if (address >= 0x8000 && address <= 0x9FFF)
    {
        // Disable and reset IRQ counter
        m_IRQEnabled = false;
        m_IRQActive = false;
        m_IRQCounter = 0;
    }
    else if (address >= 0xA000 && address <= 0xBFFF)
    {
        // Enable IRQ Counter
        m_IRQEnabled = true;
        m_IRQCounter = 4096u;
    }
    else if (address >= 0xC000 && address <= 0xDFFF)
    {
        // Nothing here
    }
    else if (address >= 0xE000 && address <= 0xFFFF)
    {
        // Select switchable bank. There is only 8 of them
        m_switchableBank = data & 0x07;
        UpdateMapping();
    }

    return false;
}

bool Mapper_040::MapReadPPU(uint16_t address, uint32_t& mappedAddress, uint8_t& /*data*/)
{
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        mappedAddress = address;
        return true;
    }

    return false;
}

bool Mapper_040::MapWritePPU(uint16_t /*address*/, uint32_t& /*mappedAddress*/, uint8_t /*data*/) { return false; }

void Mapper_040::InternalReset()
{
    m_switchableBank = 0;
    m_IRQEnabled = false;
    m_IRQActive = false;
    m_IRQCounter = 0;

    m_mapping.m_prgMapping = {4, 5, m_switchableBank, 7};
    m_mapping.m_chrMapping = {0, 1, 2, 3, 4, 5, 6, 7};
    m_mapping.m_prgRamMapping.fill(6);
    m_mapping.m_ramIsProgram = true;
}

void Mapper_040::CPUClock()
{
    if (m_IRQCounter == 0 || !m_IRQEnabled)
    {
        return;
    }

    if (--m_IRQCounter == 0 && m_IRQEnabled)
    {
        m_IRQActive = true;
    }
}

bool Mapper_040::ShouldIRQ() const { return m_IRQActive; }

void Mapper_040::ClearIRQ() { m_IRQActive = false; }

void Mapper_040::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    IMapper::SerializeTo(visitor);

    visitor.WriteValue(m_switchableBank);
    visitor.WriteValue(m_IRQCounter);
    visitor.WriteValue(m_IRQEnabled);
    visitor.WriteValue(m_IRQActive);
}

void Mapper_040::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    IMapper::DeserializeFrom(visitor);

    visitor.ReadValue(m_switchableBank);
    visitor.ReadValue(m_IRQCounter);
    visitor.ReadValue(m_IRQEnabled);
    visitor.ReadValue(m_IRQActive);

    UpdateMapping();
}

inline void Mapper_040::UpdateMapping() { m_mapping.m_prgMapping[2] = m_switchableBank; }