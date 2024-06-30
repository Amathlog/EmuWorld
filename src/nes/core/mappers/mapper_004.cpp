#include "core/mapper.h"
#include <cassert>
#include <core/mappers/mapper_004.h>
#include <cstdint>

using NesEmulator::Mapper_004;
using NesEmulator::Mapping;

Mapper_004::Mapper_004(const iNESHeader& header, Mapping& mapping)
    : IMapper(header, mapping)
{
    InternalReset();
}

bool Mapper_004::MapReadCPU(uint16_t address, uint32_t& mappedAddress, uint8_t& data)
{
    if (address >= 0x8000 && address <= 0x9FFF)
    {
        // First bank
        // On mode 0, it is index 6 (R6)
        // On mode 1, it is the second-to-last bank
        // Multiply the nb of prgbanks because it is 16kB chunks, but we address 8kB at a time.
        if (m_prgRomBankMode == 0)
            mappedAddress = m_indexes[6] * 0x2000 + (address & 0x1FFF);
        else
            mappedAddress = (m_nbPrgBanks * 2 - 2) * 0x2000 + (address & 0x1FFF);

        return true;
    }
    else if (address >= 0xA000 && address <= 0xBFFF)
    {
        // Second bank
        // On mode 0 and 1, it is always the index 7 (R7)
        mappedAddress = m_indexes[7] * 0x2000 + (address & 0x1FFF);
        return true;
    }
    else if (address >= 0xC000 && address <= 0xDFFF)
    {
        // Third bank
        // On mode 0, it is the second-to-last
        // On mode 1, it is the index 6 (R6)
        if (m_prgRomBankMode == 1)
            mappedAddress = m_indexes[6] * 0x2000 + (address & 0x1FFF);
        else
            mappedAddress = (m_nbPrgBanks * 2 - 2) * 0x2000 + (address & 0x1FFF);

        return true;
    }
    else if (address >= 0xE000 && address <= 0xFFFF)
    {
        // Fourth bank
        // It is always the last bank
        mappedAddress = (m_nbPrgBanks * 2 - 1) * 0x2000 + (address & 0x1FFF);
        return true;
    }

    return false;
}

bool Mapper_004::MapWriteCPU(uint16_t address, uint32_t& mappedAddress, uint8_t data)
{
    if (address >= 0x8000 && address <= 0x9FFF)
    {
        if (address & 0x0001)
        {
            // Change the value of the selected index.
            // Ignore top 2 bits on R6 and R7 (only 64 prg rom banks), and ignore bottom bit
            // on R0 and R1 (since it is 2kB instead of 1kB for chr rom index)
            // If there are more than 64 prg rom banks (like some rom hacks), we can use the top
            // 2 bits to handle up to 256 prg rom banks.
            // Only use them if there are more than 64 prg rom banks
            if (m_nextIndexToUpdate >= 6 && m_nbPrgBanks <= 64)
            {
                data &= 0x3F;
            }
            else if (m_nextIndexToUpdate <= 1)
            {
                data &= 0xFE;
            }

            m_indexes[m_nextIndexToUpdate] = data;
            UpdateMapping();
        }
        else
        {
            // First 3 bits define the next index to update
            m_nextIndexToUpdate = data & 0x07;

            // 6th bit for the prg mode
            m_prgRomBankMode = (data & 0x40) > 0;

            // 7th bit for the chr mode
            m_chrRomBankMode = (data & 0x80) > 0;
        }
    }
    else if (address >= 0xA000 && address <= 0xBFFF)
    {
        if (address & 0x0001)
        {
            // Don't support write protection
            // PrgRam is on bit 7
            m_prgRamEnabled = (data & 0x80) > 0;
            m_mapping.m_ramEnabled = m_prgRamEnabled;
        }
        else
        {
            // Mirroring on bit 0, only if we are not in four screen mode
            if (m_mirroring != Mirroring::FOUR_SCREEN)
            {
                m_mirroring = (data & 0x01) ? Mirroring::HORIZONTAL : Mirroring::VERTICAL;
            }
        }
    }
    else if (address >= 0xC000 && address <= 0xDFFF)
    {
        if (address & 0x0001)
        {
            m_IRQShouldReload = true;
        }
        else
        {
            m_IRQReload = data;
        }
    }
    else if (address >= 0xE000 && address <= 0xFFFF)
    {
        if (address & 0x0001)
        {
            m_IRQEnabled = true;
        }
        else
        {
            m_IRQEnabled = false;
            m_IRQActive = false;
        }
    }

    return false;
}

bool Mapper_004::MapReadPPU(uint16_t address, uint32_t& mappedAddress, uint8_t& /*data*/)
{
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        // On mode 0, it is R0, R1 (with 2kB each) and then R2, R3, R4 and R5 (with 1kB each)
        // On mode 1, it is R2, R3, R4, R5 (with 1kB each) and then R0, R1 (with 2kB each)
        bool isZeroAndOneIndex = (address <= 0x0FFF) ^ m_chrRomBankMode;
        uint8_t index = 0;

        if (isZeroAndOneIndex)
        {
            index = (address & 0x0FFF) >= 0x0800;
            address &= 0x07FF;
        }
        else
        {
            // Limits are 0x000, 0x400, 0x800 and 0xC00
            // (address & 0x0C00) >> 10 will give us 0, 1, 2 or 3
            // by adding 2, we have R2, R3, R4 and R5
            index = 2 + ((address & 0x0C00) >> 10);
            address &= 0x03FF;
        }

        mappedAddress = m_indexes[index] * 0x0400 + address;
        return true;
    }

    return false;
}

bool Mapper_004::MapWritePPU(uint16_t address, uint32_t& mappedAddress, uint8_t data)
{
    return MapReadPPU(address, mappedAddress, data);
}

void Mapper_004::InternalReset()
{
    m_indexes.fill(0);
    m_prgRomBankMode = 0;
    m_chrRomBankMode = 0;
    m_nextIndexToUpdate = 0;
    m_prgRamEnabled = true;
    m_IRQEnabled = false;
    m_IRQActive = false;
    m_IRQCounter = 0;
    m_IRQReload = 0;

    UpdateMapping();
}

void Mapper_004::ScanlineDone()
{
    if (m_IRQCounter == 0 || m_IRQShouldReload)
    {
        m_IRQCounter = m_IRQReload;
        m_IRQShouldReload = false;
    }
    else
    {
        m_IRQCounter--;
    }

    if (m_IRQCounter == 0 && m_IRQEnabled)
    {
        m_IRQActive = true;
    }
}

bool Mapper_004::ShouldIRQ() const { return m_IRQActive; }

void Mapper_004::ClearIRQ() { m_IRQActive = false; }

void Mapper_004::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    IMapper::SerializeTo(visitor);

    visitor.Write(m_indexes.data(), m_indexes.size());
    visitor.WriteValue(m_prgRomBankMode);
    visitor.WriteValue(m_chrRomBankMode);
    visitor.WriteValue(m_nextIndexToUpdate);
    visitor.WriteValue(m_prgRamEnabled);
    visitor.WriteValue(m_IRQEnabled);
    visitor.WriteValue(m_IRQActive);
    visitor.WriteValue(m_IRQCounter);
    visitor.WriteValue(m_IRQReload);
}

void Mapper_004::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    IMapper::DeserializeFrom(visitor);

    visitor.Read(m_indexes.data(), m_indexes.size());
    visitor.ReadValue(m_prgRomBankMode);
    visitor.ReadValue(m_chrRomBankMode);
    visitor.ReadValue(m_nextIndexToUpdate);
    visitor.ReadValue(m_prgRamEnabled);
    visitor.ReadValue(m_IRQEnabled);
    visitor.ReadValue(m_IRQActive);
    visitor.ReadValue(m_IRQCounter);
    visitor.ReadValue(m_IRQReload);

    UpdateMapping();
}

void Mapper_004::UpdateMapping()
{
    m_mapping.m_ramEnabled = m_prgRamEnabled;

    m_mapping.m_prgMapping[0] = m_prgRomBankMode == 0 ? m_indexes[6] : 2 * (m_nbPrgBanks - 1);
    m_mapping.m_prgMapping[1] = m_indexes[7];
    m_mapping.m_prgMapping[2] = m_prgRomBankMode == 1 ? m_indexes[6] : 2 * (m_nbPrgBanks - 1);
    m_mapping.m_prgMapping[3] = 2 * m_nbPrgBanks - 1;

    // On mode 0, it is R0, R1 (with 2kB each) and then R2, R3, R4 and R5 (with 1kB each)
    // On mode 1, it is R2, R3, R4, R5 (with 1kB each) and then R0, R1 (with 2kB each)
    if (m_chrRomBankMode == 0)
    {
        m_mapping.m_chrMapping[0] = m_indexes[0];
        m_mapping.m_chrMapping[1] = m_indexes[0] + 1;
        m_mapping.m_chrMapping[2] = m_indexes[1];
        m_mapping.m_chrMapping[3] = m_indexes[1] + 1;
        m_mapping.m_chrMapping[4] = m_indexes[2];
        m_mapping.m_chrMapping[5] = m_indexes[3];
        m_mapping.m_chrMapping[6] = m_indexes[4];
        m_mapping.m_chrMapping[7] = m_indexes[5];
    }
    else
    {
        m_mapping.m_chrMapping[0] = m_indexes[2];
        m_mapping.m_chrMapping[1] = m_indexes[3];
        m_mapping.m_chrMapping[2] = m_indexes[4];
        m_mapping.m_chrMapping[3] = m_indexes[5];
        m_mapping.m_chrMapping[4] = m_indexes[0];
        m_mapping.m_chrMapping[5] = m_indexes[0] + 1;
        m_mapping.m_chrMapping[6] = m_indexes[1];
        m_mapping.m_chrMapping[7] = m_indexes[1] + 1;
    }

    // RAM is fixed
    m_mapping.m_prgRamMapping.fill(0);
}