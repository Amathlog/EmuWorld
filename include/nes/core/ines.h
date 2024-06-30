#pragma once
#include <cstdint>
#include <core/constants.h>
#include <cstring>

namespace NesEmulator 
{
    union Flag6
    {
        struct
        {
            uint8_t mirroring:1;
            uint8_t hasPersistentMemory:1;
            uint8_t hasTrainerData:1;
            uint8_t ignoreMirroringControl:1;
            uint8_t lowerNibbleMapperNumber:4;
        };

        uint8_t flag;
    };

    union Flag7
    {
        struct
        {
            uint8_t vsUnisystem:1;
            uint8_t playChoice:1;
            uint8_t iNes2Identifier:2;
            uint8_t upperNibbleMapperNumber:4;
        };

        uint8_t flag;
    };

    union Flag8
    {
        // iNes 2.0 format
        struct
        {
            uint8_t superUpperNibbleMapperNumber:4;
            uint8_t submapperNumber:4;
        };

        // iNes 1.0 format
        uint8_t prgRamSize;

        uint8_t flag;
    };

    union Flag9
    {
        // iNes 2.0 format
        struct
        {
            uint8_t prgRomSizeMsb:4;
            uint8_t chrRomSizeMsb:4;
        };

        // iNes 1.0 format
        struct
        {
            uint8_t tvSystem:1;
            uint8_t unused:7;
        };

        uint8_t flag;
    };

    union Flag10
    {
        // iNes 2.0 format
        struct
        {
            uint8_t prgRamShiftCount:4;
            uint8_t prgNVRamShiftCount:4;
        };

        // iNes 1.0 format
        struct
        {
            uint8_t tvSystem:2;
            uint8_t unused:2;
            uint8_t hasPrgRam:1;
            uint8_t boardHasBusConflicts:1;
            uint8_t unused2:2;
        };

        uint8_t flag;
    };

    union Flag11
    {
        struct
        {
            uint8_t chrRamShiftCount:4;
            uint8_t chrNVRamShiftCount:4;
        };

        uint8_t flag;
    };

    union Flag12
    {
        struct
        {
            uint8_t timingMode:2;
            uint8_t unused:6;
        };

        uint8_t flag;
    };

    union Flag13
    {
        // When it is VS system type
        struct
        {
            uint8_t vsPPUType:4;
            uint8_t vsHardwareType:4;
        };

        // When it is Extended console type
        struct
        {
            uint8_t extendedConsoleType:4;
            uint8_t unused:4;
        };

        uint8_t flag;
    };

    union Flag14
    {
        struct
        {
            uint8_t miscellaneousROMs:2;
            uint8_t unused:6;
        };

        uint8_t flag;
    };

    union Flag15
    {
        struct
        {
            uint8_t defaultExpansionDevice:6;
            uint8_t unused:2;
        };

        uint8_t flag;
    };

    struct iNESHeader
    {
        char nesName[4];
        uint8_t prgRomSize; // lsb on iNes 2.0
        uint8_t chrRomSize; // lsb on iNes 2.0
        Flag6 flag6;
        Flag7 flag7;
        Flag8 flag8;
        Flag9 flag9;
        Flag10 flag10;
        Flag11 flag11;
        Flag12 flag12;
        Flag13 flag13;
        Flag14 flag14;
        Flag15 flag15;

        bool isValid() const { return strncmp(nesName, NesEmulator::Cst::NES_HEADER, 4) == 0; }
        bool isINES2() const { return flag7.iNes2Identifier == 2; }

        uint16_t GetMapperId() const 
        {
            uint16_t mapperId = (flag7.upperNibbleMapperNumber << 4) | flag6.lowerNibbleMapperNumber;
            if (isINES2())
                mapperId |= (flag8.superUpperNibbleMapperNumber << 8);

            return mapperId;
        }

        uint16_t GetPRGROMSize() const { return !isINES2() ? prgRomSize : ((flag9.prgRomSizeMsb << 8) | prgRomSize); }
        uint16_t GetCHRROMSize() const { return !isINES2() ? chrRomSize : ((flag9.chrRomSizeMsb << 8) | chrRomSize); }
        uint16_t GetPRGRAMSize() const { return !isINES2() ? flag8.prgRamSize : (64 << flag10.prgRamShiftCount); }
        uint16_t GetCHRRAMSize() const { return !isINES2() ? 0 : (64 << flag11.chrRamShiftCount); }
        uint16_t GetPRGNVRAMSize() const { return !isINES2() ? 0 : (64 << flag10.prgNVRamShiftCount); }
        uint16_t GetCHRNVRAMSize() const { return !isINES2() ? 0 : (64 << flag11.chrNVRamShiftCount); }

        NesEmulator::Mode GetRegion() const
        {
            if (isINES2())
            {
                switch(flag12.timingMode)
                {
                case 1:
                    return NesEmulator::Mode::PAL;
                default:
                    // Only support PAL and NTSC for now
                    return NesEmulator::Mode::NTSC;
                }
            }
            else
            {
                // Not the most reliable it seems
                uint8_t value = (flag9.tvSystem) | (flag10.tvSystem >> 1);
                return NesEmulator::Mode(value);
            }
        }
    };
}