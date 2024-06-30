#pragma once

#include <core/utils/busVisitor.h>
#include <cstdint>
#include <string>
#include <vector>

namespace NesEmulator
{
namespace Utils
{
class IReadVisitor;

std::vector<std::string> Disassemble(BusReadVisitor& busVisitor, uint16_t wantedPC, uint16_t& indexOfWantedPC);
} // namespace Utils
} // namespace NesEmulator