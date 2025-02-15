#pragma once

#include <array>
#include <common/utils/utils.h>
#include <core/constants.h>
#include <core/serializable.h>
#include <queue>
#include <vector>

namespace GBEmulator
{
class Bus;

enum class InteruptSource
{
    HBlank,
    VBlank,
    OAM,
    LYC
};

union LCDRegister
{
    struct
    {
        uint8_t BGAndWindowPriority : 1;
        uint8_t objEnable : 1;
        uint8_t objSize : 1;
        uint8_t bgTileMapArea : 1;
        uint8_t BGAndWindowTileAreaData : 1;
        uint8_t windowEnable : 1;
        uint8_t windowTileMapArea : 1;
        uint8_t enable : 1;
    };

    uint8_t flags = 0x00;
};

union LCDStatus
{
    struct
    {
        uint8_t mode : 2;
        uint8_t lYcEqualLY : 1;
        // Interrupt sources (IS)
        uint8_t mode0HBlankIS : 1;
        uint8_t mode1VBlankIS : 1;
        uint8_t mode2OAMIS : 1;
        uint8_t lYcEqualLYIS : 1;
        uint8_t unused : 1;
    };

    uint8_t flags = 0x00;
};

union GBPaletteData
{
    struct
    {
        uint8_t color0 : 2;
        uint8_t color1 : 2;
        uint8_t color2 : 2;
        uint8_t color3 : 2;
    };

    uint8_t flags = 0x00;
};

struct GBCPaletteData
{
    std::array<RGB555, 4> colors;

    void Reset();

    void SerializeTo(Common::Utils::IWriteVisitor& visitor) const;
    void DeserializeFrom(Common::Utils::IReadVisitor& visitor);
};

struct GBCPaletteAccess
{
    bool shouldIncr = false;
    uint8_t address = 0x00;

    void Reset()
    {
        shouldIncr = false;
        address = 0x00;
    }
};

struct PixelFIFO
{
    uint8_t color = 0x00;
    uint8_t palette = 0x00;
    uint8_t bgPriority = 0x00;
};

union Attributes
{
    struct
    {
        uint8_t paletteNumberGBC : 3;
        uint8_t tileVRAMBank : 1;
        uint8_t paletteNumberGB : 1;
        uint8_t xFlip : 1;
        uint8_t yFlip : 1;
        uint8_t bgAndWindowOverObj : 1;
    };

    uint8_t flags = 0x00;
};

struct OAMEntry
{
    uint8_t xPosition = 0x00;
    uint8_t yPosition = 0x00;
    uint8_t tileIndex = 0x00;

    Attributes attributes;
};

class Processor2C02 : public ISerializable
{
public:
    Processor2C02();

    uint8_t ReadByte(uint16_t addr, bool readOnly = false);
    void WriteByte(uint16_t addr, uint8_t data);

    void SerializeTo(Common::Utils::IWriteVisitor& visitor) const override;
    void DeserializeFrom(Common::Utils::IReadVisitor& visitor) override;

    constexpr unsigned GetHeight() const { return GB_INTERNAL_HEIGHT; }
    constexpr unsigned GetWidth() const { return GB_INTERNAL_WIDTH; }
    const auto& GetScreen() const { return m_screen; }

    const auto& GetOAMEntries() const { return m_OAM; }
    GBPaletteData GetBGPalette() const { return m_gbBGPalette; }
    GBPaletteData GetOAM0Palette() const { return m_gbOBJ0Palette; }
    GBPaletteData GetOAM1Palette() const { return m_gbOBJ1Palette; }

    const auto& GetBGPalettesGBC() const { return m_gbcBGPalettes; }
    const auto& GetOBJPalettesGBC() const { return m_gbcOBJPalettes; }

    bool IsFrameComplete() const { return m_isFrameComplete; }
    bool IsInHBlank() const { return m_lcdStatus.mode == 0; }

    void ConnectBus(Bus* bus) { m_bus = bus; }

    const std::vector<uint8_t>& GetVRAM() const { return m_VRAM; }

    void Reset();
    void Clock();

    using GBCPaletteDataArray = std::array<GBCPaletteData, 8>;

    uint8_t GetLY() const { return m_lY; }

private:
    void DebugRenderNoise();
    void DebugRenderTileIds();
    void RenderPixelFifos();
    void RenderDisabledLCD();
    void SetInteruptFlag(InteruptSource is);

    void OriginalPixelFetcher();
    void SimplifiedPixelFetcher();

    uint8_t ReadVRAM(uint16_t addr, uint8_t bankNumber);
    void WriteVRAM(uint16_t addr, uint8_t bankNumber, uint8_t data);

    Bus* m_bus = nullptr;
    LCDRegister m_lcdRegister;
    LCDStatus m_lcdStatus;

    uint8_t m_scrollY = 0x00;
    uint8_t m_scrollX = 0x00;
    uint8_t m_lY = 0x00;
    uint8_t m_lYC = 0x00;
    uint8_t m_wY = 0x00;
    uint8_t m_wX = 0x00;
    uint8_t m_windowStalling = 0x00;

    GBPaletteData m_gbBGPalette;
    GBPaletteData m_gbOBJ0Palette;
    GBPaletteData m_gbOBJ1Palette;

    // GBC Spcific
    bool m_isGBC = false;
    GBCPaletteDataArray m_gbcBGPalettes;
    GBCPaletteDataArray m_gbcOBJPalettes;
    GBCPaletteAccess m_gbcBGPaletteAccess;
    GBCPaletteAccess m_gbcOBJPaletteAccess;

    // FIFOs
    Common::Utils::MyStaticQueue<PixelFIFO, 160> m_bgFifo;
    Common::Utils::MyStaticQueue<PixelFIFO, 160> m_objFifo;

    std::array<PixelFIFO, 8> m_currentFetchedBGPixels;
    std::array<PixelFIFO, 8> m_currentFetchedOBJPixels;

    // OAM
    // 4 bytes per entry, 40 entries
    std::array<OAMEntry, 40> m_OAM;
    std::vector<uint8_t> m_selectedOAM;

    uint8_t m_currentStagePixelFetcher = 0x00;
    uint8_t m_XOffsetBGTile = 0x00;
    uint16_t m_BGWindowTileAddress = 0x0000;
    uint8_t m_initialBGXScroll = 0x00;
    bool m_isWindowRendering = false;

    // Counters
    unsigned m_lineDots = 0x00;
    unsigned m_scanlines = 0x00;
    unsigned m_currentLinePixel = 0x00;
    uint8_t m_currentX = 0x00;
    uint8_t m_currentNbPixelsToRender = 0x00;

    // Screen
    std::vector<uint8_t> m_screen;
    bool m_isFrameComplete = false;
    bool m_isDisabled = false;

    // VRAM
    std::vector<uint8_t> m_VRAM;
    uint8_t m_currentVRAMBank;
};
} // namespace GBEmulator