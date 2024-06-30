#include <core/cartridge.h>
#include <core/constants.h>
#include <core/ines.h>
#include <core/mappers/all_mappers.h>

#include <common/utils/sha1.h>
#include <common/utils/visitor.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

using Common::Utils::IReadVisitor;
using NesEmulator::Cartridge;

Cartridge::~Cartridge() {}

Cartridge::Cartridge(IReadVisitor& visitor)
{
    // First read the header
    iNESHeader header;
    visitor.Read(&header, 1);

    // Check that the first bytes are the one we expect. If not, perhaps it is not the right
    // format so abort there
    assert(header.isValid() && "The NES header is what we expected. Aborting.");

    // If there is trainer data, ignore it
    if (header.flag6.hasTrainerData)
        visitor.Advance(NesEmulator::Cst::ROM_TRAINER_SIZE);

    m_nbPrgBanks = header.GetPRGROMSize();
    m_prgData.resize(m_nbPrgBanks * NesEmulator::Cst::ROM_PRG_CHUNK_SIZE);
    visitor.Read(m_prgData.data(), m_prgData.size());

    m_nbChrBanks = header.GetCHRROMSize();
    if (m_nbChrBanks > 0)
    {
        m_chrData.resize(m_nbChrBanks * NesEmulator::Cst::ROM_CHR_CHUNK_SIZE);
        visitor.Read(m_chrData.data(), m_chrData.size());
    }
    else
    {
        m_chrData.resize(0x2000);
    }

    m_vRam.resize(0x2000);

    // Setup the mapper
    m_mapper = NesEmulator::CreateMapper(header, m_mapping);
    m_useVRam = m_mapper->GetMirroring() == Mirroring::FOUR_SCREEN;

    assert(m_mapper.get() != nullptr && "Invalid mapper id, unsupported");

    // Allocate 32kB of prgRam
    m_prgRam.resize(0x8000);

    // When all is done, compute the SHA1 of the ROM
    Common::Utils::SHA1 sha1;
    sha1.update(m_prgData);
    sha1.update(m_chrData);
    m_sha1 = sha1.final();
}

bool Cartridge::ReadCPU(uint16_t address, uint8_t& data)
{
    uint32_t mappedAddress = 0;
    if (address >= 0x6000 && address <= 0x7FFF && m_mapping.m_ramEnabled)
    {
        mappedAddress = m_mapping.m_prgRamMapping[0] * 0x2000 + (address & 0x1FFF);
        data = m_mapping.m_ramIsProgram ? m_prgData[mappedAddress] : m_prgRam[mappedAddress];
        return true;
    }

    if (address >= 0x8000)
    {
        uint16_t index = ((address & 0x7000) >> 13);
        mappedAddress = m_mapping.m_prgMapping[index] * 0x2000 + (address & 0x1FFF);
        data = m_prgData[mappedAddress];
        return true;
    }

    return false;
}

bool Cartridge::WriteCPU(uint16_t addr, uint8_t data)
{
    uint32_t mappedAddress = 0;
    if (addr >= 0x6000 && addr <= 0x7FFF && !m_mapping.m_ramIsProgram && m_mapping.m_ramEnabled)
    {
        mappedAddress = m_mapping.m_prgRamMapping[0] * 0x2000 + (addr & 0x1FFF);
        m_prgRam[mappedAddress] = data;
        return true;
    }

    if (m_mapper->MapWriteCPU(addr, mappedAddress, data))
    {
        if (mappedAddress == 0xFFFFFFFF)
            return true;

        m_prgData[mappedAddress] = data;
        return true;
    }

    return false;
}

bool Cartridge::WritePPU(uint16_t addr, uint8_t data)
{
    uint32_t mappedAddress = 0;
    // First check if we are currently in Four screen mode, and in the vram/palette range
    if (m_useVRam && addr >= Cst::PPU_START_VRAM && addr <= Cst::PPU_END_PALETTE)
    {
        m_vRam[addr - Cst::PPU_START_VRAM] = data;
        return true;
    }

    if (addr <= 0x1FFF)
    {
        uint16_t index = (addr >> 10);
        mappedAddress = m_mapping.m_chrMapping[index] * 0x0400 + (addr & 0x03FF);
        m_chrData[mappedAddress] = data;
        return true;
    }

    return false;
}

bool Cartridge::ReadPPU(uint16_t addr, uint8_t& data)
{
    uint32_t mappedAddress = 0;

    // First check if we are currently in Four screen mode, and in the vram/palette range
    if (m_useVRam && addr >= Cst::PPU_START_VRAM && addr <= Cst::PPU_END_PALETTE)
    {
        data = m_vRam[addr - Cst::PPU_START_VRAM];
        return true;
    }

    if (addr <= 0x1FFF)
    {
        uint16_t index = (addr >> 10);
        mappedAddress = m_mapping.m_chrMapping[index] * 0x0400 + (addr & 0x03FF);
        data = m_chrData[mappedAddress];
        return true;
    }

    return false;
}

void Cartridge::Reset()
{
    m_mapping.Reset();

    if (m_mapper)
        m_mapper->Reset();
}

void Cartridge::SaveRAM(Common::Utils::IWriteVisitor& visitor) const { visitor.WriteContainer(m_prgRam); }

void Cartridge::LoadRAM(Common::Utils::IReadVisitor& visitor) { visitor.ReadContainer(m_prgRam); }

void Cartridge::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    SaveRAM(visitor);
    if (m_mapper)
        m_mapper->SerializeTo(visitor);

    // Check if we have chr rom, if not, we need to serialize it
    if (m_nbChrBanks == 0)
        visitor.WriteContainer(m_chrData);
}

void Cartridge::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    LoadRAM(visitor);
    if (m_mapper)
        m_mapper->DeserializeFrom(visitor);

    if (m_nbChrBanks == 0)
        visitor.ReadContainer(m_chrData);
}