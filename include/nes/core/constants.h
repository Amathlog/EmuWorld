#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

using std::size_t;

namespace NesEmulator
{
    enum Mode : unsigned
    {
        NTSC = 0,
        PAL = 1,

        COUNT = 2
    };

namespace Cst
{
    // Bus addresses
    constexpr uint16_t RAM_START_ADDR = 0x0000;
    constexpr uint16_t RAM_END_ADDR = 0x1FFF;
    constexpr uint16_t RAM_SIZE = 0x0800; // 8kB addr range but only 2kB RAM. Mirroring

    constexpr uint16_t PPU_REG_START_ADDR = 0x2000;
    constexpr uint16_t PPU_REG_END_ADDR = 0x3FFF;
    constexpr uint16_t PPU_REG_SIZE = 0x0008; // 8kB addr range, but only 8 bytes usable. Mirroring

    constexpr uint16_t APU_IO_START_ADDR = 0x4000;
    constexpr uint16_t APU_IO_END_ADDR = 0x4017;
    constexpr uint16_t APU_IO_SIZE = APU_IO_END_ADDR - APU_IO_START_ADDR;

    constexpr uint16_t ROM_START_ADDR = 0x8000;
    constexpr uint16_t ROM_END_ADDR = 0xFFFF;
    constexpr uint16_t ROM_SIZE = ROM_END_ADDR - ROM_START_ADDR;

    // Roms constants, following iNES format (https://wiki.nesdev.org/w/index.php/INES)
    constexpr char NES_HEADER[5] = "\x4E\x45\x53\x1A";
    constexpr uint16_t ROM_PRG_CHUNK_SIZE = 0x4000;
    constexpr uint16_t ROM_CHR_CHUNK_SIZE = 0x2000;

    constexpr uint16_t ROM_TRAINER_SIZE = 0x200;

    // PPU specific
    constexpr uint8_t PPU_NB_NAMED_TABLES = 0x02;
    constexpr uint16_t PPU_NAMED_TABLES_SIZE = 0x0400;
    constexpr uint8_t PPU_PALETTE_SIZE = 0x20;

    constexpr uint16_t PPU_START_CHR_ROM = 0x0000;
    constexpr uint16_t PPU_END_CHR_ROM = 0x1FFF;
    constexpr uint16_t PPUT_CHR_ROM_SIZE = PPU_END_CHR_ROM - PPU_START_CHR_ROM;

    constexpr uint16_t PPU_START_VRAM = 0x2000;
    constexpr uint16_t PPU_END_VRAM = 0x3EFF;
    constexpr uint16_t PPUT_VRAM_SIZE = PPU_END_VRAM - PPU_START_VRAM;

    constexpr uint16_t PPU_START_PALETTE = 0x3F00;
    constexpr uint16_t PPU_END_PALETTE = 0x3FFF;

    constexpr uint16_t PPU_MASK_MIRROR = 0x3FFF;

    // Sizes
    constexpr uint16_t SCREEN_WIDTH = 256;
    constexpr uint16_t SCREEN_HEIGHT = 240;

    constexpr uint16_t CONTROLLER_1_ADDR = 0x4016;
    constexpr uint16_t CONTROLLER_2_ADDR = 0x4017;

    constexpr uint16_t DMA_REGISTER_ADDR = 0x4014;

    constexpr double NTSC_MASTER_CLOCK = 21477272.0;
    constexpr double PAL_MASTER_CLOCK = 26601712.0;
    constexpr double NTSC_CPU_FREQUENCY = NTSC_MASTER_CLOCK / 12;
    constexpr double NTSC_PPU_FREQUENCY = NTSC_MASTER_CLOCK / 4;
    constexpr double PAL_CPU_FREQUENCY = PAL_MASTER_CLOCK / 16;
    constexpr double PAL_PPU_FREQUENCY = PAL_MASTER_CLOCK / 5;

    // APU specific
    constexpr uint16_t APU_SEQUENCER_STEP1 = 3729;
    constexpr uint16_t APU_SEQUENCER_STEP2 = 7457;
    constexpr uint16_t APU_SEQUENCER_STEP3 = 11186;
    constexpr uint16_t APU_SEQUENCER_STEP4 = 14915;
    constexpr uint16_t APU_SEQUENCER_STEP5 = 18640;

    constexpr std::array<uint8_t, 32> APU_LENGTH_TABLE = 
    {
      10, 254, 20,  2, 40,  4, 80,  6,
      160,  8, 60, 10, 14, 12, 26, 14,
      12,  16, 24, 18, 48, 20, 96, 22,
      192, 24, 72, 26, 16, 28, 32, 30 
    };

    constexpr std::array<int16_t, 16> APU_NOISE_PERIOD_NTSC = 
    {
        4,   8,   16,  32,  64,  96,   128,  160,
        202, 254, 380, 508, 762, 1016, 2034, 4068
    };

    constexpr std::array<int16_t, 16> APU_NOISE_PERIOD_PAL = 
    {
        4,   8,   14,  30,  60,  88,  118,  148,
        188, 236, 354, 472, 708, 944, 1890, 3778
    };

    // Values are originally in CPU cycles, change them to APU cycles
    // so divide them by 2
    constexpr std::array<int16_t, 16> APU_DMC_RATE_NTSC =
    {
        428 / 2, 380 / 2, 340 / 2, 320 / 2, 286 / 2, 254 / 2, 226 / 2, 214 / 2,
        190 / 2, 160 / 2, 142 / 2, 128 / 2, 106 / 2,  84 / 2,  72 / 2,  54 / 2
    };

    constexpr std::array<int16_t, 16> APU_DMC_RATE_PAL =
    {
        398 / 2, 354 / 2, 316 / 2, 298 / 2, 276 / 2, 236 / 2, 210 / 2, 198 / 2,
        176 / 2, 148 / 2, 132 / 2, 118 / 2, 98 / 2,  78 / 2,  66 / 2,  50 / 2
    };

    constexpr double SAMPLE_RATE = 44100.0;
}
}