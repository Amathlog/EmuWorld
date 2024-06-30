#pragma once

#include "core/processor2C02Registers.h"
#include <array>
#include <common/utils/visitor.h>
#include <core/constants.h>
#include <core/serializable.h>
#include <cstdint>
#include <memory>

namespace NesEmulator
{
class Cartridge;

class Processor2C02 : public ISerializable
{
public:
    Processor2C02();
    ~Processor2C02() = default;

    void WriteCPU(uint16_t addr, uint8_t data);
    uint8_t ReadCPU(uint16_t addr);

    void WritePPU(uint16_t addr, uint8_t data);
    uint8_t ReadPPU(uint16_t addr);

    void ConnectCartridge(const std::shared_ptr<Cartridge>& cartridge) { m_cartridge = cartridge; }
    void Clock();
    void Reset();

    const uint8_t* GetScreen() const { return reinterpret_cast<const uint8_t*>(m_screen); }
    constexpr unsigned GetWidth() const { return 256u; }
    constexpr unsigned GetHeight() const { return 240u; }
    bool IsFrameComplete() const { return m_isFrameComplete; }

    int16_t GetScanlines() const { return m_scanlines; }
    int16_t GetCycles() const { return m_cycles; }

    void RandomizeScreen();
    uint8_t GetColorFromPaletteRam(uint8_t n, uint8_t i);
    void FillFromPatternTable(uint8_t index, uint8_t selectedPalette, uint8_t* buffer);

    void FillFromNameTable(uint8_t index, uint8_t selectedPalette, uint8_t* buffer, bool switchIndexes = false);
    void CompleteReadOfNameTables(uint8_t* buffer);

    bool IsNMISet() const { return m_nmi; }
    void ResetNMI() { m_nmi = false; }

    const PPURegisters& GetRegisters() const { return m_registers; }

    void SerializeTo(Common::Utils::IWriteVisitor& visitor) const override;
    void DeserializeFrom(Common::Utils::IReadVisitor& visitor) override;

private:
    std::shared_ptr<Cartridge> m_cartridge;

    // Memory
    std::array<std::array<uint8_t, Cst::PPU_NAMED_TABLES_SIZE>, Cst::PPU_NB_NAMED_TABLES> m_namedTables;
    std::array<uint8_t, Cst::PPU_PALETTE_SIZE> m_paletteTable;

    uint8_t m_screen[256 * 240];
    bool m_isFrameComplete = false;
    bool m_nmi = false;

    PPURegisters m_registers;

    int16_t m_scanlines = 0;
    int16_t m_cycles = 0;

    std::array<OAM, 8> m_selectedSprites = {};
    uint8_t m_spritesCount = 0;
};
} // namespace NesEmulator