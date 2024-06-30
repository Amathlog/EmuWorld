#pragma once

#include <common/utils/visitor.h>
#include <core/constants.h>
#include <core/ines.h>
#include <core/serializable.h>
#include <cstdint>
#include <vector>

namespace NesEmulator
{
enum class Mirroring
{
    VERTICAL,
    HORIZONTAL,
    ONESCREEN_LO,
    ONESCREEN_HI,
    FOUR_SCREEN
};

struct Mapping
{
    // PRG rom is located between 0x8000 and 0xFFFF (32kB)
    // We cut this range in chunks of 8kB (0x2000), that can be changed
    // indicating where we need to reed in program data.
    // We do this to avoid virtual calls to IMapper, which is costly
    // if done a lot of times.
    std::array<uint16_t, 4> m_prgMapping;

    // Same thing for CHR rom, between 0x0000 and 0x1FFF (8kB)
    // Cut in chunks of 1kB (0x0400)
    std::array<uint16_t, 8> m_chrMapping;

    // And same thing for PRG RAM, between 0x6000 and 0x7FFF (8kB)
    // but can also be extended to use PRG ROM address range (up to 0xFFFF)
    // Cut in chunks of 8kB
    std::array<uint16_t, 5> m_prgRamMapping;

    // Extra parameters for more custom mappers
    bool m_ramIsProgram = false;
    bool m_ramEnabled = true;

    void Reset()
    {
        m_prgMapping.fill(0);
        m_chrMapping.fill(0);
        m_prgRamMapping.fill(0);
        m_ramIsProgram = false;
        m_ramEnabled = true;
    }
};

class IMapper : public ISerializable
{
public:
    IMapper(const iNESHeader& header, Mapping& mapping)
        : m_header(header)
        , m_nbPrgBanks(m_header.GetPRGROMSize())
        , m_nbChrBanks(m_header.GetCHRROMSize())
        , m_id(header.GetMapperId())
        , m_mapping(mapping)
    {
        if (m_header.flag6.ignoreMirroringControl)
            m_originalMirroring = Mirroring::FOUR_SCREEN;
        else
            m_originalMirroring = (m_header.flag6.mirroring) ? Mirroring::VERTICAL : Mirroring::HORIZONTAL;

        m_mirroring = m_originalMirroring;
    }

    virtual ~IMapper() = default;

    virtual bool MapReadCPU(uint16_t address, uint32_t& mappedAddress, uint8_t& data) = 0;
    virtual bool MapWriteCPU(uint16_t address, uint32_t& mappedAddress, uint8_t data) = 0;
    virtual bool MapReadPPU(uint16_t address, uint32_t& mappedAddress, uint8_t& data) = 0;
    virtual bool MapWritePPU(uint16_t address, uint32_t& mappedAddress, uint8_t data) = 0;
    Mirroring GetMirroring() const { return m_mirroring; }

    NesEmulator::Mode GetMode() const { return m_header.GetRegion(); }
    uint16_t GetMapperId() const { return m_id; }

    virtual void Reset() { m_mirroring = m_originalMirroring; }

    // Functions specific for the PPU. Handle IRQ from the mapper. Used for mapper 004 for example.
    // It is not very accurate but will do the job nonetheless
    // By default, all those functions does nothing. Each mapper can handle this as they please

    // Called at the end of each scanline
    virtual void ScanlineDone() {}
    // Called at the end for each PPU cycle
    virtual void CPUClock() {}
    // Check if we should fire an IRQ
    virtual bool ShouldIRQ() const { return false; }
    // Clear the IRQ state
    virtual void ClearIRQ() {}

    bool HasPersistantMemory() const { return m_header.flag6.hasPersistentMemory; }

    void SerializeTo(Common::Utils::IWriteVisitor& visitor) const override { visitor.WriteValue(m_mirroring); }

    void DeserializeFrom(Common::Utils::IReadVisitor& visitor) override { visitor.ReadValue(m_mirroring); }

protected:
    uint16_t m_id;
    iNESHeader m_header;
    uint16_t m_nbPrgBanks;
    uint16_t m_nbChrBanks;
    Mirroring m_originalMirroring;
    Mirroring m_mirroring;

    Mapping& m_mapping;
};
} // namespace NesEmulator