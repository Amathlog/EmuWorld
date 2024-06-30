#include <cassert>
#include <common/utils/utils.h>
#include <common/utils/visitor.h>
#include <core/processor6502.h>
#include <core/utils/disassembly.h>
#include <iostream>
#include <sstream>
#include <string>

using Common::Utils::Hex;
using Common::Utils::IReadVisitor;
using NesEmulator::Processor6502;

std::vector<std::string> NesEmulator::Utils::Disassemble(BusReadVisitor& busVisitor, uint16_t wantedPC,
                                                         uint16_t& indexOfWantedPC)
{
    std::vector<std::string> res;
    // Dummy cpu
    Processor6502 dummyCPU;
    auto opCodeMapper = dummyCPU.GetOpCodeMapper();

    while (busVisitor.Remaining() > 0)
    {
        std::stringstream newValue;
        uint16_t currentPC = busVisitor.GetCurrentPtr();

        Hex(newValue, currentPC, 4, "0x", " ");

        if (currentPC == wantedPC)
            indexOfWantedPC = (uint16_t)res.size();

        // First read the first byte, it's the opcode
        uint8_t opCode;
        busVisitor.Read(&opCode, 1);
        currentPC++;

        const Processor6502::Instruction& instruction = opCodeMapper[opCode];

        newValue << instruction.name << " ";

        uint16_t operand = 0;

        // Depending on the addressing mode, we need to read 0, 1 or 2 operands
        if (instruction.addrmode == &Processor6502::IMP)
        {
            // Nothing to read
        }
        else if (instruction.addrmode == &Processor6502::ABS || instruction.addrmode == &Processor6502::ABX ||
                 instruction.addrmode == &Processor6502::ABY || instruction.addrmode == &Processor6502::IND)
        {
            if (busVisitor.Remaining() < 2)
                break;

            // 2 bytes to read, it's an address
            busVisitor.Read(&operand, 1);
            currentPC += 2;
        }
        else
        {
            if (busVisitor.Remaining() < 1)
                break;

            // 1 byte to read
            uint8_t data;
            busVisitor.Read(&data, 1);
            operand = data;
            currentPC++;
        }

        // Depending on the addr mode, we output something different
        if (instruction.addrmode == &Processor6502::IMP)
        {
            // Nothing to do
        }
        else if (instruction.addrmode == &Processor6502::IMM)
        {
            Hex(newValue, operand, 2, "#");
        }
        else if (instruction.addrmode == &Processor6502::ZP0)
        {
            Hex(newValue, operand, 2, "$");
        }
        else if (instruction.addrmode == &Processor6502::ZPX)
        {
            Hex(newValue, operand, 2, "$", ",X");
        }
        else if (instruction.addrmode == &Processor6502::ZPY)
        {
            Hex(newValue, operand, 2, "$", ",Y");
        }
        else if (instruction.addrmode == &Processor6502::REL)
        {
            operand = currentPC + (int8_t)(operand);
            Hex(newValue, operand, 4, "$[", "]");
        }
        else if (instruction.addrmode == &Processor6502::ABS)
        {
            Hex(newValue, operand, 4, "$");
        }
        else if (instruction.addrmode == &Processor6502::ABX)
        {
            Hex(newValue, operand, 4, "$", ",X");
        }
        else if (instruction.addrmode == &Processor6502::ABY)
        {
            Hex(newValue, operand, 4, "$", ",Y");
        }
        else if (instruction.addrmode == &Processor6502::IND)
        {
            Hex(newValue, operand, 4, "($", ")");
        }
        else if (instruction.addrmode == &Processor6502::IZX)
        {
            Hex(newValue, operand, 2, "($", ",X)");
        }
        else if (instruction.addrmode == &Processor6502::IZY)
        {
            Hex(newValue, operand, 2, "($", "),Y");
        }

        res.push_back(newValue.str());
    }

    return res;
}