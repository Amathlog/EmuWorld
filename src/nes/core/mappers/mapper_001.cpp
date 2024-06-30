#include "core/mapper.h"
#include <cassert>
#include <core/mappers/mapper_001.h>
#include <cstdint>

using NesEmulator::Mapper_001;
using NesEmulator::Mapping;

Mapper_001::Mapper_001(const iNESHeader& header, Mapping& mapping)
    : IMapper(header, mapping)
{
    // We need to be sure that there is 1 or 2 prgBanks and 1 chrBanks
    assert(m_nbPrgBanks >= 2 && m_nbPrgBanks <= 32 && (m_nbChrBanks == 0 || m_nbChrBanks >= 2) && m_nbChrBanks <= 32 &&
           "Wrong number of prgBanks or chrBans in mapper 001");

    InternalReset();
}

bool Mapper_001::MapReadCPU(uint16_t address, uint32_t& mappedAddress, uint8_t& data)
{
    // Should not be called

    if (address >= 0x8000)
    {
        if (m_32kBModePrgBank)
        {
            mappedAddress = m_currentPrgBankSwitch * 0x8000 + (address & 0x7FFF);
        }
        else
        {
            uint8_t bankNumber = 0;
            if (address <= 0xBFFF)
            {
                // Low bank
                // If bank 0 is fixed, it is the first one
                bankNumber = m_PrgBank0IsSwitch ? m_currentPrgBankSwitch : 0;
            }
            else
            {
                // High bank
                // If bank 1 is fixed, it is the last one
                bankNumber = !m_PrgBank0IsSwitch ? m_currentPrgBankSwitch : m_nbPrgBanks - 1;
            }

            mappedAddress = bankNumber * 0x4000 + (address & 0x3FFF);
        }
        return true;
    }

    return false;
}

bool Mapper_001::MapWriteCPU(uint16_t address, uint32_t& mappedAddress, uint8_t data)
{
    if (address >= 0x8000)
    {
        // In this case, we need to load some data in the shift register.
        // The address doesn't mean anything.
        // It's when the shift register is full that the address is important.
        HandleLoadRegister(data);
        if (m_loadRegisterDone)
        {
            // Control (0x8000-0x9FFF)
            if (address <= 0x9FFF)
            {
                uint8_t mirroring = m_internalRegister & 0x03;
                uint8_t prgROMBank = (m_internalRegister >> 2) & 0x03;
                uint8_t chrROMBank = (m_internalRegister >> 4) & 0x01;

                // First check if we switch our prg ROM bank
                m_32kBModePrgBank = prgROMBank <= 1;
                if (prgROMBank == 2)
                {
                    // Fix the bank 0 to the first bank in prg data
                    m_PrgBank0IsSwitch = false;
                }
                else if (prgROMBank == 3)
                {
                    // Fix the bank 1 to the last bank in prg data
                    m_PrgBank0IsSwitch = true;
                }

                // Second check for CHR bank
                m_8kBModeChrBank = chrROMBank == 0;

                // Finally check the mirroring
                // Only done if we are not in Four screen mode
                if (m_mirroring != Mirroring::FOUR_SCREEN)
                {
                    switch (mirroring)
                    {
                    case 0:
                        m_mirroring = Mirroring::ONESCREEN_LO;
                        break;
                    case 1:
                        m_mirroring = Mirroring::ONESCREEN_HI;
                        break;
                    case 2:
                        m_mirroring = Mirroring::VERTICAL;
                        break;
                    case 3:
                        m_mirroring = Mirroring::HORIZONTAL;
                        break;
                    default:
                        break;
                    }
                }

                // Perhaps not necessary at this stage
                UpdateMapping();
            }
            // CHR Bank 0 (0xA000-0xBFFF)
            else if (address <= 0xBFFF)
            {
                uint8_t chrBank0 = m_internalRegister & 0x1F;
                // Ignore lowest bit in 8kB mode
                m_currentChrBank0 = m_8kBModeChrBank ? (chrBank0 >> 1) : chrBank0;
                UpdateMapping();
            }
            // CHR Bank 1 (0xC000-0xDFFF)
            else if (address <= 0xDFFF)
            {
                uint8_t chrBank1 = m_internalRegister & 0x1F;
                // Ignored in 8kB mode
                if (!m_8kBModeChrBank)
                {
                    m_currentChrBank1 = chrBank1;
                }
                UpdateMapping();
            }
            // PRG Bank (0xE000-0xFFFF)
            else
            {
                uint8_t prgROMBank = m_internalRegister & 0x0F;
                // uint8_t prgRAMChipEnable = (m_internalRegister & 0x10) > 0;

                // Low bit ignored in 32kB mode
                m_currentPrgBankSwitch =
                    (m_currentPrgBankSwitch & 0x10) | (m_32kBModePrgBank ? (prgROMBank >> 1) : prgROMBank);
                UpdateMapping();
            }

            m_loadRegisterDone = false;
        }
    }

    return false;
}

bool Mapper_001::MapReadPPU(uint16_t address, uint32_t& mappedAddress, uint8_t& /*data*/)
{
    // Should not be called
    // First check if we have Chr bank. If not, we need to read from RAM
    if (address <= 0x1FFF)
    {
        if (m_nbChrBanks == 0)
        {
            mappedAddress = address;
            return true;
        }

        if (m_8kBModeChrBank)
        {
            mappedAddress = m_currentChrBank0 * 0x2000 + (address & 0x1FFF);
        }
        else
        {
            uint8_t bankNumber = address <= 0x0FFF ? m_currentChrBank0 : m_currentChrBank1;
            mappedAddress = 0x1000 * bankNumber + (address & 0x0FFF);
        }

        return true;
    }

    return false;
}

bool Mapper_001::MapWritePPU(uint16_t address, uint32_t& mappedAddress, uint8_t /*data*/)
{
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        if (m_nbChrBanks == 0)
            mappedAddress = address;

        return true;
    }

    return false;
}

void Mapper_001::ResetShiftRegister()
{
    m_shiftRegister = 0x00;
    m_shiftCounter = 0;
}

void Mapper_001::ResetControlRegister()
{
    m_PrgBank0IsSwitch = true;
    m_32kBModePrgBank = false;
    m_8kBModeChrBank = false;
    m_mirroring = m_originalMirroring;
}

void Mapper_001::HandleLoadRegister(uint8_t data)
{
    // If the highest bit is set, we just reset
    if ((data & 0x80) > 0)
    {
        ResetShiftRegister();
        // Control register is also reset
        ResetControlRegister();
        return;
    }

    // We receive bits, starting with LSB. So we need this shift gymnastic.
    m_shiftRegister |= ((data & 0x01) << m_shiftCounter);

    if (++m_shiftCounter == 5)
    {
        m_loadRegisterDone = true;
        m_internalRegister = m_shiftRegister & 0x1F;
        ResetShiftRegister();
    }
}

void Mapper_001::InternalReset()
{
    ResetShiftRegister();
    m_internalRegister = 0x00;
    m_loadRegisterDone = false;

    m_currentPrgBankSwitch = 0;
    m_currentChrBank0 = 0;
    m_currentChrBank1 = 0;
    ResetControlRegister();

    UpdateMapping();
}

void Mapper_001::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    IMapper::SerializeTo(visitor);

    visitor.WriteValue(m_shiftRegister);
    visitor.WriteValue(m_shiftCounter);
    visitor.WriteValue(m_internalRegister);
    visitor.WriteValue(m_loadRegisterDone);

    visitor.WriteValue(m_currentPrgBankSwitch);
    visitor.WriteValue(m_PrgBank0IsSwitch);
    visitor.WriteValue(m_currentChrBank0);
    visitor.WriteValue(m_currentChrBank1);
    visitor.WriteValue(m_32kBModePrgBank);
    visitor.WriteValue(m_8kBModeChrBank);
}

void Mapper_001::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    IMapper::DeserializeFrom(visitor);

    visitor.ReadValue(m_shiftRegister);
    visitor.ReadValue(m_shiftCounter);
    visitor.ReadValue(m_internalRegister);
    visitor.ReadValue(m_loadRegisterDone);

    visitor.ReadValue(m_currentPrgBankSwitch);
    visitor.ReadValue(m_PrgBank0IsSwitch);
    visitor.ReadValue(m_currentChrBank0);
    visitor.ReadValue(m_currentChrBank1);
    visitor.ReadValue(m_32kBModePrgBank);
    visitor.ReadValue(m_8kBModeChrBank);

    UpdateMapping();
}

void Mapper_001::UpdateMapping()
{
    // PRG
    if (m_32kBModePrgBank)
    {
        uint16_t currentBank = (uint16_t)m_currentPrgBankSwitch * 4;
        m_mapping.m_prgMapping = {currentBank, (uint16_t)(currentBank + 1), (uint16_t)(currentBank + 2),
                                  (uint16_t)(currentBank + 3)};
    }
    else
    {
        uint16_t firstBank = m_PrgBank0IsSwitch ? (uint16_t)m_currentPrgBankSwitch * 2 : 0;
        uint16_t lastBank = !m_PrgBank0IsSwitch ? (uint16_t)m_currentPrgBankSwitch * 2 : 2 * (m_nbPrgBanks - 1);
        m_mapping.m_prgMapping = {firstBank, (uint16_t)(firstBank + 1), lastBank, (uint16_t)(lastBank + 1)};
    }

    // CHR
    // If there is no CHR ROM, no mapping is done
    if (m_nbChrBanks == 0)
    {
        m_mapping.m_chrMapping = {0, 1, 2, 3, 4, 5, 6, 7};
    }
    else if (m_8kBModeChrBank)
    {
        uint16_t currentBank = m_currentChrBank0 * 8;
        m_mapping.m_chrMapping = {currentBank,
                                  (uint16_t)(currentBank + 1),
                                  (uint16_t)(currentBank + 2),
                                  (uint16_t)(currentBank + 3),
                                  (uint16_t)(currentBank + 4),
                                  (uint16_t)(currentBank + 5),
                                  (uint16_t)(currentBank + 6),
                                  (uint16_t)(currentBank + 7)};
    }
    else
    {
        uint16_t currentBank0 = m_currentChrBank0 * 4;
        uint16_t currentBank1 = m_currentChrBank1 * 4;
        m_mapping.m_chrMapping = {
            currentBank0, (uint16_t)(currentBank0 + 1), (uint16_t)(currentBank0 + 2), (uint16_t)(currentBank0 + 3),
            currentBank1, (uint16_t)(currentBank1 + 1), (uint16_t)(currentBank1 + 2), (uint16_t)(currentBank1 + 3)};
    }

    // RAM, fixed
    m_mapping.m_prgRamMapping.fill(0);
}