#pragma once

#include "core/cartridge.h"
#include "core/ines.h"
#include <memory>
#include <core/mappers/mapper_000.h>
#include <core/mappers/mapper_001.h>
#include <core/mappers/mapper_002.h>
#include <core/mappers/mapper_003.h>
#include <core/mappers/mapper_004.h>
#include "core/mappers/mapper_040.h"
#include <core/mappers/mapper_066.h>

namespace NesEmulator
{
    inline std::unique_ptr<IMapper> CreateMapper(const iNESHeader& header, Mapping& mapping)
    {
        uint16_t mapperId = header.GetMapperId();
        switch(mapperId)
        {
        case 0:
            return std::make_unique<Mapper_000>(header, mapping);
        case 1:
            return std::make_unique<Mapper_001>(header, mapping);        
        case 2:
            return std::make_unique<Mapper_002>(header, mapping);        
        case 3:
            return std::make_unique<Mapper_003>(header, mapping);
        case 4:
            return std::make_unique<Mapper_004>(header, mapping);
        case 40:
            return std::make_unique<Mapper_040>(header, mapping);
        case 66:
            return std::make_unique<Mapper_066>(header, mapping);
        default:
            return nullptr;
        }
    }
}