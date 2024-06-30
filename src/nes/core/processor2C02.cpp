#include "core/constants.h"
#include "core/mapper.h"
#include "core/processor2C02Registers.h"
#include "core/processor6502.h"
#include <core/cartridge.h>
#include <core/processor2C02.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>

using NesEmulator::Mirroring;
using NesEmulator::Processor2C02;

namespace
{
uint8_t GetNametableIndex(Mirroring mirroring, uint16_t addr)
{
    switch (mirroring)
    {
    case Mirroring::ONESCREEN_LO:
        return 0;
    case Mirroring::ONESCREEN_HI:
        return 1;
    case Mirroring::HORIZONTAL:
        return (addr & 0x0800) > 0;
    case Mirroring::VERTICAL:
        return (addr & 0x0400) > 0;
    default:
        return ((addr & 0x0C00) >> 6);
    }
}
} // namespace

Processor2C02::Processor2C02() { Reset(); }

uint8_t Processor2C02::ReadCPU(uint16_t addr)
{
    uint8_t data = 0;
    switch (addr)
    {
    case 0:
        // Can't read from Ctrl register
        break;
    case 1:
        // Can't read from Mask register
        break;
    case 2:
        // HACK: Uncomment if you want to force the verticalBlank state
        // m_registers.status.verticalBlankStarted = 1;
        data = (m_registers.status.flags & 0xE0) | (m_registers.data & 0x1F);
        m_registers.status.verticalBlankStarted = 0;
        m_registers.addr = 0;
        break;
    case 3:
        // Can't read from OAMAddress register
        break;
    case 4:
        data = m_registers.GetOAM()[m_registers.oamaddr];
        break;
    case 5:
        // Can't read from Scroll register
        break;
    case 6:
        // Can't read from Addr register
        break;
    case 7:
    {
        // delay read
        data = m_registers.data;
        m_registers.data = ReadPPU(m_registers.vramAddr.reg);

        // Note: In case of reading to the palette ram, there is no delay
        // Therefore skip the delay if we are in this range
        if (m_registers.vramAddr.reg >= Cst::PPU_START_PALETTE)
            data = m_registers.data;

        m_registers.vramAddr.reg += (m_registers.ctrl.VRAMAddressIncrement ? 32 : 1);

        break;
    }
    }

    return data;
}

void Processor2C02::WriteCPU(uint16_t addr, uint8_t data)
{
    // We never write directly to the vram address, we only write to the tram address.
    // cf loopy_registers
    // We only write to vram when we read the whole address (case 6)
    switch (addr)
    {
    case 0:
        m_registers.ctrl.flags = data;
        m_registers.tramAddr.nametableX = m_registers.ctrl.msbXScroll;
        m_registers.tramAddr.nametableY = m_registers.ctrl.msbYScroll;
        break;
    case 1:
        m_registers.mask.flags = data;
        break;
    case 2:
        // Can't write to Status register
        break;
    case 3:
        m_registers.oamaddr = data;
        break;
    case 4:
        m_registers.GetOAM()[m_registers.oamaddr] = data;
        break;
    case 5:
        // For the scrolling part, the 3 lsb are for fine and the 5 msb for coarse
        // Writing is in 2 parts, first for X, then for Y. Use the same addr flag
        // than for the address (case 6)
        if (m_registers.addr == 0)
        {
            m_registers.fineX = data & 0x07;
            m_registers.tramAddr.coarseX = data >> 3;
            m_registers.addr = 1;
        }
        else
        {
            m_registers.tramAddr.fineY = data & 0x07;
            m_registers.tramAddr.coarseY = data >> 3;
            m_registers.addr = 0;
        }
        break;
    case 6:
        if (m_registers.addr == 0)
        {
            m_registers.tramAddr.reg = (m_registers.tramAddr.reg & 0x00FF) | ((uint16_t)((data & 0x3F)) << 8);
            m_registers.addr = 1;
        }
        else
        {
            m_registers.tramAddr.reg = (m_registers.tramAddr.reg & 0xFF00) | data;
            m_registers.addr = 0;
            m_registers.vramAddr.reg = m_registers.tramAddr.reg;
        }
        break;
    case 7:
        WritePPU(m_registers.vramAddr.reg, data);
        m_registers.vramAddr.reg += (m_registers.ctrl.VRAMAddressIncrement ? 32 : 1);
        break;
    }
}

void Processor2C02::WritePPU(uint16_t addr, uint8_t data)
{
    addr &= Cst::PPU_MASK_MIRROR;
    if (m_cartridge->WritePPU(addr, data))
    {
        // Nothing to do
    }
    else if (addr >= Cst::PPU_START_CHR_ROM && addr <= Cst::PPU_END_CHR_ROM)
    {
        m_namedTables[(addr & 0x1000) >> 12][addr & 0x0FBF] = data;
    }
    else if (addr >= Cst::PPU_START_VRAM && addr <= Cst::PPU_END_VRAM)
    {
        addr &= 0x0FFF;
        m_namedTables[GetNametableIndex(m_cartridge->GetMirroring(), addr)][addr & 0x03FF] = data;
    }
    else if (addr >= Cst::PPU_START_PALETTE && addr <= Cst::PPU_END_PALETTE)
    {
        addr &= 0x001F;
        // Mirroring, to review
        if (addr == 0x0010)
            addr = 0x0000;
        if (addr == 0x0014)
            addr = 0x0004;
        if (addr == 0x0018)
            addr = 0x0008;
        if (addr == 0x001C)
            addr = 0x000C;

        m_paletteTable[addr] = data;
    }
}

uint8_t Processor2C02::ReadPPU(uint16_t addr)
{
    uint8_t data = 0;
    addr &= Cst::PPU_MASK_MIRROR;
    if (m_cartridge->ReadPPU(addr, data))
    {
        // Nothing to do
    }
    else if (addr >= Cst::PPU_START_CHR_ROM && addr <= Cst::PPU_END_CHR_ROM)
    {
        data = m_namedTables[(addr & 0x1000) >> 12][addr & 0x0FBF];
    }
    else if (addr >= Cst::PPU_START_VRAM && addr <= Cst::PPU_END_VRAM)
    {
        addr &= 0x0FFF;
        data = m_namedTables[GetNametableIndex(m_cartridge->GetMirroring(), addr)][addr & 0x03FF];
    }
    else if (addr >= Cst::PPU_START_PALETTE && addr <= Cst::PPU_END_PALETTE)
    {
        addr &= 0x001F;
        // Mirroring, to review
        if (addr == 0x0010)
            addr = 0x0000;
        if (addr == 0x0014)
            addr = 0x0004;
        if (addr == 0x0018)
            addr = 0x0008;
        if (addr == 0x001C)
            addr = 0x000C;

        data = m_paletteTable[addr] & (m_registers.mask.greyscale ? 0x30 : 0x3F);
    }
    return data;
}

void Processor2C02::Clock()
{
    m_isFrameComplete = false;
    // Utility lambdas
    auto incrementScrollX = [this]()
    {
        // We only increment the scrolling if we are rendering something
        if (m_registers.mask.showBackground || m_registers.mask.showSprites)
        {
            if (m_registers.vramAddr.coarseX == 31)
            {
                // If we reach 32, it means we are outside our current nametable.
                // (There are only 32 tiles on the X axis)
                // Therefore we need to start reading from the other one.
                // We start reading at tile 0
                m_registers.vramAddr.coarseX = 0;
                // Select the other nametable on the X axis (aka flip the bit)
                m_registers.vramAddr.nametableX = ~m_registers.vramAddr.nametableX;
            }
            else
            {
                m_registers.vramAddr.coarseX++;
            }
        }
    };

    auto incrementScrollY = [this]()
    {
        // We only increment the scrolling if we are rendering something
        if (m_registers.mask.showBackground || m_registers.mask.showSprites)
        {
            // On the Y axis, it is a bit different, as we are operating on a scanline
            // so we increment our fine Y
            // If we go beyond 7 for fineY, we increment our tile counter on the Y axis (coarseY)
            // And if we go beyond 30, it means we are outside our current nametable, so same thing as X.
            if (m_registers.vramAddr.fineY < 7)
            {
                m_registers.vramAddr.fineY++;
            }
            else
            {
                m_registers.vramAddr.fineY = 0;
                if (m_registers.vramAddr.coarseY == 29)
                {
                    m_registers.vramAddr.coarseY = 0;
                    m_registers.vramAddr.nametableY = ~m_registers.vramAddr.nametableY;
                }
                else if (m_registers.vramAddr.coarseY == 31)
                {
                    // If for some reason we are ouside our range, just wraps around
                    m_registers.vramAddr.coarseY = 0;
                }
                else
                {
                    m_registers.vramAddr.coarseY++;
                }
            }
        }
    };

    // Transfer the info from tram to vram
    auto transferAddressX = [&]()
    {
        if (m_registers.mask.showBackground || m_registers.mask.showSprites)
        {
            m_registers.vramAddr.nametableX = m_registers.tramAddr.nametableX;
            m_registers.vramAddr.coarseX = m_registers.tramAddr.coarseX;
        }
    };

    // Transfer the info from tram to vram
    auto transferAddressY = [&]()
    {
        if (m_registers.mask.showBackground || m_registers.mask.showSprites)
        {
            m_registers.vramAddr.fineY = m_registers.tramAddr.fineY;
            m_registers.vramAddr.nametableY = m_registers.tramAddr.nametableY;
            m_registers.vramAddr.coarseY = m_registers.tramAddr.coarseY;
        }
    };

    auto LoadBackgroundShifters = [&]()
    {
        m_registers.bgShifterPatternLsb = (m_registers.bgShifterPatternLsb & 0xFF00) | m_registers.bgNextTileLsb;
        m_registers.bgShifterPatternMsb = (m_registers.bgShifterPatternMsb & 0xFF00) | m_registers.bgNextTileMsb;
        // Set the whole byte to 0 or 1 depending on the tile attr value (for each bit 0 and 1)
        m_registers.bgShifterAttrLsb =
            (m_registers.bgShifterAttrLsb & 0xFF00) | ((m_registers.bgNextTileAttr & 0b01) ? 0xFF : 0x00);
        m_registers.bgShifterAttrMsb =
            (m_registers.bgShifterAttrMsb & 0xFF00) | ((m_registers.bgNextTileAttr & 0b10) ? 0xFF : 0x00);
    };

    auto UpdateShifters = [&]()
    {
        if (m_registers.mask.showBackground)
        {
            m_registers.bgShifterPatternLsb <<= 1;
            m_registers.bgShifterPatternMsb <<= 1;
            m_registers.bgShifterAttrLsb <<= 1;
            m_registers.bgShifterAttrMsb <<= 1;
        }

        if (m_registers.mask.showSprites && m_cycles >= 1 && m_cycles < 258)
        {
            for (uint8_t i = 0; i < m_spritesCount; ++i)
            {
                // Decrement X each time, when we hit 0, it means we reach the sprite,
                // From then, we can shift the shifters
                if (m_selectedSprites[i].x > 0)
                {
                    m_selectedSprites[i].x--;
                }
                else
                {
                    m_registers.fgShifterPatternLsb[i] <<= 1;
                    m_registers.fgShifterPatternMsb[i] <<= 1;
                }
            }
        }
    };

    // Visible lines (except for -1)
    if (m_scanlines >= -1 && m_scanlines < 240)
    {
        if (m_scanlines == 0 && m_cycles == 0)
        {
            // First cycle skipped
            m_cycles = 1;
            return;
        }

        if (m_scanlines == -1 && m_cycles == 1)
        {
            // End of vertical blank
            m_registers.status.verticalBlankStarted = 0;

            // Also some cleanup on sprite data
            m_registers.status.spriteOverflow = 0;
            m_registers.status.sprite0Hit = 0;

            m_spritesCount = 0;
            m_registers.fgShifterPatternLsb.fill(0x00);
            m_registers.fgShifterPatternMsb.fill(0x00);
            m_cycles++;
            return;
        }

        if ((m_cycles >= 2 && m_cycles < 258) || (m_cycles >= 321 && m_cycles < 338))
        {
            UpdateShifters();
            switch ((m_cycles - 1) % 8)
            {
            // Load nametable byte
            case 0:
                LoadBackgroundShifters();
                m_registers.bgNextTileId = ReadPPU(Cst::PPU_START_VRAM | (m_registers.vramAddr.reg & 0x0FFF));
                break;
            // Load attribute byte
            // Attribute memory is offset from the beginning of the nametable by 0x03C0
            case 2:
            {
                uint16_t adding = (m_registers.vramAddr.nametableY << 11) | (m_registers.vramAddr.nametableX << 10) |
                                  ((m_registers.vramAddr.coarseY >> 2) << 3) | (m_registers.vramAddr.coarseX >> 2);

                uint16_t ppuAddr = (Cst::PPU_START_VRAM + 0x03C0) | adding;
                m_registers.bgNextTileAttr = ReadPPU(ppuAddr);

                if (m_registers.bgNextTileAttr != 0)
                    adding = adding;

                if (m_registers.vramAddr.coarseY & 0x02)
                    m_registers.bgNextTileAttr >>= 4;
                if (m_registers.vramAddr.coarseX & 0x02)
                    m_registers.bgNextTileAttr >>= 2;
                m_registers.bgNextTileAttr &= 0x03;
                break;
            }
            // Load LSB tile byte
            case 4:
                m_registers.bgNextTileLsb =
                    ReadPPU((((uint16_t)m_registers.ctrl.backgroundPatternTableAddress) << 12) +
                            (((uint16_t)m_registers.bgNextTileId) << 4) + (m_registers.vramAddr.fineY) + 0);
                break;
            // Load MSB tile byte
            case 6:
                m_registers.bgNextTileMsb =
                    ReadPPU((((uint16_t)m_registers.ctrl.backgroundPatternTableAddress) << 12) +
                            (((uint16_t)m_registers.bgNextTileId) << 4) + (m_registers.vramAddr.fineY) + 8);
                break;
            // Increment horizontal direction
            case 7:
                incrementScrollX();
                break;
            }
        }

        // Increment vertical direction
        if (m_cycles == 256)
        {
            incrementScrollY();
        }

        // Need to reset X address after incrementing Y
        if (m_cycles == 257)
        {
            LoadBackgroundShifters();
            transferAddressX();
        }

        if (m_cycles == 338 || m_cycles == 340)
        {
            m_registers.bgNextTileId = ReadPPU(Cst::PPU_START_VRAM | (m_registers.vramAddr.reg & 0x0FFF));
        }

        // Need to reset the Y adress when we are in "pre-redering"
        // and the specification says that we do it each tick between 280 and 304 cycles
        if (m_scanlines == -1 && (m_cycles >= 280 && m_cycles <= 304))
        {
            transferAddressY();
        }

        // Foreground rendering. To improve for better compability with other games
        // For this time, we just do the whole "scanning" at a single clock
        if (m_scanlines >= 0 && m_cycles == 257)
        {
            m_registers.spriteZeroIsPossible = false;

            for (auto& item : m_selectedSprites)
                item.Reset();

            m_registers.fgShifterPatternLsb.fill(0x00);
            m_registers.fgShifterPatternMsb.fill(0x00);

            m_spritesCount = 0;
            bool spriteOverflow = false;

            // Looking for all objects, check if they are on the current scanline
            for (uint8_t i = 0; i < 64 && m_spritesCount < 9; ++i)
            {
                const OAM& oam = m_registers.oam[i];
                int16_t diff = ((int16_t)m_scanlines) - ((int16_t)m_registers.oam[i].y);
                if (diff >= 0 && diff < (m_registers.ctrl.spriteSize ? 16 : 8))
                {
                    if (m_spritesCount < 8)
                    {
                        // Is it sprite 0 ?
                        if (i == 0)
                            m_registers.spriteZeroIsPossible = true;

                        m_selectedSprites[m_spritesCount++] = oam;
                    }
                    else
                    {
                        spriteOverflow = true;
                        break;
                    }
                }
            }

            m_registers.status.spriteOverflow = spriteOverflow;
        }

        if (m_scanlines >= 0 && m_cycles == 340)
        {
            // Computing the foreground shifters for each sprite
            for (uint8_t i = 0; i < m_spritesCount; ++i)
            {
                uint16_t spritePatternAddrLsb = 0x0000;
                uint16_t spritePatternAddrMsb = 0x0000;
                const OAM& oam = m_selectedSprites[i];

                // Check if we flip vertically by checking the MSB of the attribute
                bool flipVertically = (oam.attribute & 0x80) > 0;

                // If so, inverse the lsb addr (0 -> 7 for normal, 7 -> 0 for flipped)
                uint8_t lsbAddr = (m_scanlines - oam.y) & 0x07;
                if (flipVertically)
                    lsbAddr = 7 - lsbAddr;

                // We can also check if it is flipped horizontally by checking the 2nd MSB bit of the attribute
                bool flipHorizontally = (oam.attribute & 0x40) > 0;

                // First check in which mode we are, 8x8 or 8x16
                if (m_registers.ctrl.spriteSize == 0)
                {
                    // 8x8 mode
                    spritePatternAddrLsb =
                        (m_registers.ctrl.spritePatternTableAddress << 12) | (oam.tileId << 4) | lsbAddr;
                }
                else
                {
                    // 8x16 mode
                    // We need to check the 7 msb bits of the tile id to know
                    // which row to select in our pattern table.
                    // In normal mode, the top half row is given by (tileID & 0xFE) << 4
                    // and we add one for the bottom half => ((tileID & 0xFE) + 1) << 4
                    // In case of flipped, we need to have the +1 for the top half
                    // and not for the bottom half. 1 << 4 = 16, so it's +16
                    // Put it on a 16bits int to avoid overflow.

                    uint16_t topHalfRow = ((oam.tileId & 0xFE) << 4);
                    uint16_t bottomHalfRow = ((oam.tileId & 0xFE) << 4);

                    if (flipVertically)
                        topHalfRow += (1 << 4);
                    else
                        bottomHalfRow += (1 << 4);

                    if (m_scanlines - oam.y < 8)
                    {
                        spritePatternAddrLsb = ((oam.tileId & 0x01) << 12) | topHalfRow | lsbAddr;
                    }
                    else
                    {
                        spritePatternAddrLsb = ((oam.tileId & 0x01) << 12) | bottomHalfRow | lsbAddr;
                    }
                }

                // At this point, we have the address of the low bytes
                // To have the address of the high bytes, we just add 8
                spritePatternAddrMsb = spritePatternAddrLsb + 8;

                m_registers.fgShifterPatternLsb[i] = ReadPPU(spritePatternAddrLsb);
                m_registers.fgShifterPatternMsb[i] = ReadPPU(spritePatternAddrMsb);

                if (flipHorizontally)
                {
                    // Taken from https://stackoverflow.com/questions/2602823
                    auto bitFlip = [](uint8_t b)
                    {
                        b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
                        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
                        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
                        return b;
                    };

                    m_registers.fgShifterPatternLsb[i] = bitFlip(m_registers.fgShifterPatternLsb[i]);
                    m_registers.fgShifterPatternMsb[i] = bitFlip(m_registers.fgShifterPatternMsb[i]);
                }
            }
        }
    }

    // On scanline 240, nothing happens
    if (m_scanlines == 240)
    {
        // Chilling
    }

    if (m_scanlines == 241 && m_cycles == 1)
    {
        m_registers.status.verticalBlankStarted = 1;
        if (m_registers.ctrl.generateNMIWhenVBI)
            m_nmi = true;
    }

    uint8_t bg_pixel = 0x00;
    uint8_t bg_palette = 0x00;

    uint8_t fg_pixel = 0x00;
    uint8_t fg_palette = 0x00;
    uint8_t fg_priority = 0x00;

    if (m_cycles >= 1 && m_cycles <= 256 && m_scanlines >= 0 && m_scanlines < 240)
    {
        if (m_registers.mask.showBackground)
        {
            uint16_t bit_mux = 0x8000 >> m_registers.fineX;

            uint8_t p0_pixel = (m_registers.bgShifterPatternLsb & bit_mux) > 0;
            uint8_t p1_pixel = (m_registers.bgShifterPatternMsb & bit_mux) > 0;
            bg_pixel = (p1_pixel << 1) | p0_pixel;

            uint8_t p0_palette = (m_registers.bgShifterAttrLsb & bit_mux) > 0;
            uint8_t p1_palette = (m_registers.bgShifterAttrMsb & bit_mux) > 0;
            bg_palette = (p1_palette << 1) | p0_palette;
        }

        if (m_registers.mask.showSprites)
        {
            m_registers.spriteZeroIsRendered = false;

            for (uint8_t i = 0; i < m_spritesCount; ++i)
            {
                if (m_selectedSprites[i].x == 0)
                {
                    uint8_t p0_pixel = (m_registers.fgShifterPatternLsb[i] & 0x80) > 0;
                    uint8_t p1_pixel = (m_registers.fgShifterPatternMsb[i] & 0x80) > 0;
                    fg_pixel = (p1_pixel << 1) | p0_pixel;

                    // 4 first colors are for the background, that's why we have a +4
                    fg_palette = (m_selectedSprites[i].attribute & 0x03) + 0x04;
                    fg_priority = (m_selectedSprites[i].attribute & 0x20) == 0;

                    // If we have a pixel to draw, we stop stop here
                    if (fg_pixel != 0)
                    {
                        // If it is our first sprite selected
                        // It is sprite zero rendered, if the first sprite selected is sprite zero
                        // (and we know it with our flag spriteZeroIsPossible)
                        if (i == 0)
                            m_registers.spriteZeroIsRendered = m_registers.spriteZeroIsPossible;

                        break;
                    }
                }
            }
        }

        // Compute the final pixel and palette!
        // By default, draw the background pixel
        uint8_t pixel = 0x00;
        uint8_t palette = 0x00;

        // If we have no background pixel, draw the foreground pixel
        // If we have both not 0, draw the pixel only if it has priority
        if (bg_pixel != 0 && fg_pixel == 0 && (m_cycles >= 9 || m_registers.mask.showBackgroundLeftmost))
        {
            pixel = bg_pixel;
            palette = bg_palette;
        }
        else if (bg_pixel == 0 && fg_pixel != 0 && (m_cycles >= 9 || m_registers.mask.showSpritesLeftmost))
        {
            pixel = fg_pixel;
            palette = fg_palette;
        }
        else if (bg_pixel != 0 && fg_pixel != 0)
        {
            // When we have both, we still need to check if we can render them if we are on the 8 first cycles.
            if (m_cycles < 9)
            {
                if (!m_registers.mask.showBackgroundLeftmost && !m_registers.mask.showSpritesLeftmost)
                {
                    fg_priority = 0;
                    bg_pixel = 0x00;
                    bg_palette = 0x00;
                }
                else if (m_registers.mask.showBackgroundLeftmost && !m_registers.mask.showSpritesLeftmost)
                {
                    fg_priority = 0;
                }
                else
                {
                    fg_priority = 1;
                }
            }

            pixel = fg_priority > 0 ? fg_pixel : bg_pixel;
            palette = fg_priority > 0 ? fg_palette : bg_palette;

            // Also, we can only have a sprite zero hit, if the sprite
            // "collides" with the background.
            if (m_registers.spriteZeroIsRendered)
            {
                // If we don't render the left most background or sprite,
                // we can't have sprite 0 hit for the 8 first cycles.
                if (~(m_registers.mask.showBackgroundLeftmost | m_registers.mask.showSpritesLeftmost))
                {
                    if (m_cycles >= 9 && m_cycles < 258)
                    {
                        m_registers.status.sprite0Hit = 1;
                    }
                }
                else
                {
                    if (m_cycles >= 1 && m_cycles < 258)
                    {
                        m_registers.status.sprite0Hit = 1;
                    }
                }
            }
        }

        size_t index = m_scanlines * 256 + m_cycles - 1;
        m_screen[index] = GetColorFromPaletteRam(palette, pixel);
    }

    m_cycles++;
    if (m_registers.mask.showBackground || m_registers.mask.showSprites)
    {
        // Tell the mapper we have finish a scanline
        if (m_cycles == 260 && m_scanlines < 240)
        {
            m_cartridge->GetMapper()->ScanlineDone();
        }
    }

    if (m_cycles >= 341)
    {
        m_cycles = 0;
        m_scanlines++;
        if (m_scanlines >= 261)
        {
            m_scanlines = -1;
            m_isFrameComplete = true;
        }
    }
}

void Processor2C02::Reset()
{
    memset(m_screen, 0, GetWidth() * GetHeight());
    m_namedTables[0].fill(0);
    m_namedTables[1].fill(0);
    m_paletteTable.fill(0);
    m_registers.Reset();
    m_scanlines = 0;
    m_cycles = 0;
    m_spritesCount = 0;
    for (auto& oam : m_selectedSprites)
        oam.Reset();
}

void Processor2C02::RandomizeScreen()
{
    for (unsigned j = 0; j < GetHeight(); ++j)
    {
        for (unsigned i = 0; i < GetWidth(); ++i)
        {
            m_screen[j * 256 + i] = rand() % 2 == 0 ? 0x30 : 0x0f;
        }
    }
}

uint8_t Processor2C02::GetColorFromPaletteRam(uint8_t n, uint8_t i)
{
    uint16_t addr = 0x3F00 + (n * 4 + i);
    return ReadPPU(addr) & 0x3F;
}

void Processor2C02::FillFromPatternTable(uint8_t index, uint8_t selectedPalette, uint8_t* buffer)
{
    // memcpy(buffer, m_namedTables[index].data(), m_namedTables[index].size());
    for (uint16_t tileY = 0; tileY < 16; ++tileY)
    {
        for (uint16_t tileX = 0; tileX < 16; ++tileX)
        {
            uint16_t offset = tileY * 256 + tileX * 16;
            for (uint8_t row = 0; row < 8; ++row)
            {
                uint16_t addr = index * 0x1000 + offset + row;
                // It seems in the rom, it stores 64 bits of lowPixel and then 64 bits of
                // highPixel. Therefore, we need to offset our read for high by 8 bytes.
                uint8_t lowPixel = ReadPPU(addr);
                uint8_t highPixel = ReadPPU(addr + 8);

                for (uint8_t col = 0; col < 8; ++col)
                {
                    uint8_t pixelValue = ((highPixel & 0x01) << 1) | (lowPixel & 0x01);
                    lowPixel >>= 1;
                    highPixel >>= 1;

                    uint16_t x = tileX * 8 + (7 - col);
                    uint16_t y = tileY * 8 + row;
                    uint16_t value = y * 128 + x;
                    uint8_t pixelColor = GetColorFromPaletteRam(selectedPalette, pixelValue);
                    buffer[value] = pixelColor;
                }
            }
        }
    }
}

void Processor2C02::FillFromNameTable(uint8_t index, uint8_t selectedPalette, uint8_t* buffer, bool switchIndexes)
{
    for (uint16_t y = 0; y < 30; ++y)
    {
        for (uint16_t x = 0; x < 32; ++x)
        {
            uint16_t position = y * 32 + x;
            uint8_t tileID = m_namedTables[index][position];

            uint8_t tileY = (tileID >> 4) & 0x0F;
            uint8_t tileX = tileID & 0x0F;

            uint16_t offset = tileY * 256 + tileX * 16;

            for (uint8_t row = 0; row < 8; ++row)
            {
                uint16_t addrNameTable = (switchIndexes ? (index + 1) % 2 : index) * 0x1000;
                uint16_t addr = addrNameTable + offset + row;
                // It seems in the rom, it stores 64 bits of lowPixel and then 64 bits of
                // highPixel. Therefore, we need to offset our read for high by 8 bytes.
                uint8_t lowPixel = ReadPPU(addr);
                uint8_t highPixel = ReadPPU(addr + 8);

                for (uint8_t col = 0; col < 8; ++col)
                {
                    uint8_t pixelValue = ((highPixel & 0x01) << 1) | (lowPixel & 0x01);
                    lowPixel >>= 1;
                    highPixel >>= 1;

                    uint16_t x_pixel = x * 8 + (7 - col);
                    uint16_t y_pixel = y * 8 + row;
                    uint16_t value = y_pixel * 256 + x_pixel;
                    uint8_t pixelColor = GetColorFromPaletteRam(selectedPalette, pixelValue);
                    buffer[value] = pixelColor;
                }
            }
        }
    }
}

void Processor2C02::CompleteReadOfNameTables(uint8_t* buffer)
{
    constexpr uint16_t nameTableWidth = 256;
    constexpr uint16_t nameTableHeight = 240;
    for (uint16_t y = 0; y < 2; ++y)
    {
        for (uint16_t x = 0; x < 2; ++x)
        {
            // Nametable addr
            uint16_t nameTableYOffset = y ? 0x0800 : 0x0000;
            uint16_t nameTableXOffset = x ? 0x0400 : 0x0000;
            uint16_t nameTableAddr = Cst::PPU_START_VRAM + nameTableYOffset + nameTableXOffset;

            uint32_t pixelXOffset = x ? nameTableWidth : 0;
            uint32_t pixelYOffset = y ? nameTableWidth * 2 * nameTableHeight : 0;

            // Then iterate on all the nametable
            for (uint16_t tileY = 0; tileY < 30; ++tileY)
            {
                for (uint16_t tileX = 0; tileX < 32; ++tileX)
                {
                    uint16_t tileAddr = nameTableAddr + (tileY << 5) + tileX;
                    uint16_t attributeOffset = ((tileY >> 2) << 3) | (tileX >> 2);
                    uint16_t attributeAddr = nameTableAddr + 0x03C0 + attributeOffset;

                    uint8_t tileId = ReadPPU(tileAddr);
                    uint8_t tileAttr = ReadPPU(attributeAddr);

                    if (tileY & 0x02)
                        tileAttr >>= 4;
                    if (tileX & 0x02)
                        tileAttr >>= 2;
                    tileAttr &= 0x03;

                    // From that, we can read the data associated with the tileId, and get the associated
                    // palette with the tileAttr

                    uint16_t tileOffset = ((tileId >> 4) & 0x0F) * 256 + (tileId & 0x0F) * 16;

                    for (uint8_t row = 0; row < 8; ++row)
                    {
                        uint16_t patternTableAddr = m_registers.ctrl.backgroundPatternTableAddress * 0x1000;
                        uint16_t addr = patternTableAddr + tileOffset + row;
                        // It seems in the rom, it stores 64 bits of lowPixel and then 64 bits of
                        // highPixel. Therefore, we need to offset our read for high by 8 bytes.
                        uint8_t lowPixel = ReadPPU(addr);
                        uint8_t highPixel = ReadPPU(addr + 8);

                        for (uint8_t col = 0; col < 8; ++col)
                        {
                            uint8_t pixelValue = ((highPixel & 0x01) << 1) | (lowPixel & 0x01);
                            lowPixel >>= 1;
                            highPixel >>= 1;

                            uint16_t x_pixel = tileX * 8 + (7 - col);
                            uint16_t y_pixel = tileY * 8 + row;
                            uint32_t value = pixelYOffset + pixelXOffset + y_pixel * 2 * nameTableWidth + x_pixel;
                            uint8_t pixelColor = GetColorFromPaletteRam(tileAttr, pixelValue);
                            buffer[value] = pixelColor;
                        }
                    }
                }
            }
        }
    }
}

void Processor2C02::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    for (const auto& nameTable : m_namedTables)
        visitor.Write(nameTable.data(), Cst::PPU_NAMED_TABLES_SIZE);

    visitor.Write(m_paletteTable.data(), Cst::PPU_PALETTE_SIZE);

    // Should not be necessary to serialize the screen
    // visitor.Write(m_screen, sizeof(m_screen));

    visitor.WriteValue(m_isFrameComplete);
    visitor.WriteValue(m_nmi);

    m_registers.SerializeTo(visitor);

    visitor.WriteValue(m_scanlines);
    visitor.WriteValue(m_cycles);

    for (const auto& oam : m_selectedSprites)
        oam.SerializeTo(visitor);

    visitor.WriteValue(m_spritesCount);
}

void Processor2C02::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    for (auto& nameTable : m_namedTables)
        visitor.Read(nameTable.data(), Cst::PPU_NAMED_TABLES_SIZE);

    visitor.Read(m_paletteTable.data(), Cst::PPU_PALETTE_SIZE);

    // Should not be necessary to serialize the screen
    // visitor.Write(m_screen, sizeof(m_screen));

    visitor.ReadValue(m_isFrameComplete);
    visitor.ReadValue(m_nmi);

    m_registers.DeserializeFrom(visitor);

    visitor.ReadValue(m_scanlines);
    visitor.ReadValue(m_cycles);

    for (auto& oam : m_selectedSprites)
        oam.DeserializeFrom(visitor);

    visitor.ReadValue(m_spritesCount);
}