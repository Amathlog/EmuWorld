#include "core/constants.h"
#include <common/utils/visitor.h>
#include <core/bus.h>
#include <core/processor6502.h>

#include <cassert>
#include <cstdint>
#include <iostream>

#define LOGGING 0

using NesEmulator::Processor6502;

enum SerializerVersion : int
{
    V1 = 0,
    Current = V1
};

Processor6502::Processor6502()
{
    using a = Processor6502;

    m_opCodeMapper = {
        {"BRK", &a::BRK, &a::IMM, 7}, {"ORA", &a::ORA, &a::IZX, 6}, {"JAM", &a::JAM, &a::IMP, 2},
        {"SLO", &a::SLO, &a::IZX, 8}, {"NOP", &a::NOP, &a::ZP0, 3}, {"ORA", &a::ORA, &a::ZP0, 3},
        {"ASL", &a::ASL, &a::ZP0, 5}, {"SLO", &a::SLO, &a::ZP0, 5}, {"PHP", &a::PHP, &a::IMP, 3},
        {"ORA", &a::ORA, &a::IMM, 2}, {"ASL", &a::ASL, &a::IMP, 2}, {"ANC", &a::ANC, &a::IMM, 2},
        {"NOP", &a::NOP, &a::ABS, 4}, {"ORA", &a::ORA, &a::ABS, 4}, {"ASL", &a::ASL, &a::ABS, 6},
        {"SLO", &a::SLO, &a::ABS, 6}, {"BPL", &a::BPL, &a::REL, 2}, {"ORA", &a::ORA, &a::IZY, 5},
        {"JAM", &a::JAM, &a::IMP, 2}, {"SLO", &a::SLO, &a::IZY, 8}, {"NOP", &a::NOP, &a::ZPX, 4},
        {"ORA", &a::ORA, &a::ZPX, 4}, {"ASL", &a::ASL, &a::ZPX, 6}, {"SLO", &a::SLO, &a::ZPX, 6},
        {"CLC", &a::CLC, &a::IMP, 2}, {"ORA", &a::ORA, &a::ABY, 4}, {"NOP", &a::NOP, &a::IMP, 2},
        {"SLO", &a::SLO, &a::ABY, 7}, {"NOP", &a::NOP, &a::ABX, 4}, {"ORA", &a::ORA, &a::ABX, 4},
        {"ASL", &a::ASL, &a::ABX, 7}, {"SLO", &a::SLO, &a::ABX, 7}, {"JSR", &a::JSR, &a::ABS, 6},
        {"AND", &a::AND, &a::IZX, 6}, {"JAM", &a::JAM, &a::IMP, 2}, {"RLA", &a::RLA, &a::IZX, 8},
        {"BIT", &a::BIT, &a::ZP0, 3}, {"AND", &a::AND, &a::ZP0, 3}, {"ROL", &a::ROL, &a::ZP0, 5},
        {"RLA", &a::RLA, &a::ZP0, 5}, {"PLP", &a::PLP, &a::IMP, 4}, {"AND", &a::AND, &a::IMM, 2},
        {"ROL", &a::ROL, &a::IMP, 2}, {"ANC", &a::ANC, &a::IMM, 2}, {"BIT", &a::BIT, &a::ABS, 4},
        {"AND", &a::AND, &a::ABS, 4}, {"ROL", &a::ROL, &a::ABS, 6}, {"RLA", &a::RLA, &a::ABS, 6},
        {"BMI", &a::BMI, &a::REL, 2}, {"AND", &a::AND, &a::IZY, 5}, {"JAM", &a::JAM, &a::IMP, 2},
        {"RLA", &a::RLA, &a::IZY, 8}, {"NOP", &a::NOP, &a::ZPX, 4}, {"AND", &a::AND, &a::ZPX, 4},
        {"ROL", &a::ROL, &a::ZPX, 6}, {"RLA", &a::RLA, &a::ZPX, 6}, {"SEC", &a::SEC, &a::IMP, 2},
        {"AND", &a::AND, &a::ABY, 4}, {"NOP", &a::NOP, &a::IMP, 2}, {"RLA", &a::RLA, &a::ABY, 7},
        {"NOP", &a::NOP, &a::ABX, 4}, {"AND", &a::AND, &a::ABX, 4}, {"ROL", &a::ROL, &a::ABX, 7},
        {"RLA", &a::RLA, &a::ABX, 7}, {"RTI", &a::RTI, &a::IMP, 6}, {"EOR", &a::EOR, &a::IZX, 6},
        {"JAM", &a::JAM, &a::IMP, 2}, {"SRE", &a::SRE, &a::IZX, 8}, {"NOP", &a::NOP, &a::ZP0, 3},
        {"EOR", &a::EOR, &a::ZP0, 3}, {"LSR", &a::LSR, &a::ZP0, 5}, {"SRE", &a::SRE, &a::ZP0, 5},
        {"PHA", &a::PHA, &a::IMP, 3}, {"EOR", &a::EOR, &a::IMM, 2}, {"LSR", &a::LSR, &a::IMP, 2},
        {"ALR", &a::ALR, &a::IMM, 2}, {"JMP", &a::JMP, &a::ABS, 3}, {"EOR", &a::EOR, &a::ABS, 4},
        {"LSR", &a::LSR, &a::ABS, 6}, {"SRE", &a::SRE, &a::ABS, 6}, {"BVC", &a::BVC, &a::REL, 2},
        {"EOR", &a::EOR, &a::IZY, 5}, {"JAM", &a::JAM, &a::IMP, 2}, {"SRE", &a::SRE, &a::IZY, 8},
        {"NOP", &a::NOP, &a::ZPX, 4}, {"EOR", &a::EOR, &a::ZPX, 4}, {"LSR", &a::LSR, &a::ZPX, 6},
        {"SRE", &a::SRE, &a::ZPX, 6}, {"CLI", &a::CLI, &a::IMP, 2}, {"EOR", &a::EOR, &a::ABY, 4},
        {"NOP", &a::NOP, &a::IMP, 2}, {"SRE", &a::SRE, &a::ABY, 7}, {"NOP", &a::NOP, &a::ABX, 4},
        {"EOR", &a::EOR, &a::ABX, 4}, {"LSR", &a::LSR, &a::ABX, 7}, {"SRE", &a::SRE, &a::ABX, 7},
        {"RTS", &a::RTS, &a::IMP, 6}, {"ADC", &a::ADC, &a::IZX, 6}, {"JAM", &a::JAM, &a::IMP, 2},
        {"RRA", &a::RRA, &a::IZX, 8}, {"NOP", &a::NOP, &a::ZP0, 3}, {"ADC", &a::ADC, &a::ZP0, 3},
        {"ROR", &a::ROR, &a::ZP0, 5}, {"RRA", &a::RRA, &a::ZP0, 5}, {"PLA", &a::PLA, &a::IMP, 4},
        {"ADC", &a::ADC, &a::IMM, 2}, {"ROR", &a::ROR, &a::IMP, 2}, {"ARR", &a::ARR, &a::IMM, 2},
        {"JMP", &a::JMP, &a::IND, 5}, {"ADC", &a::ADC, &a::ABS, 4}, {"ROR", &a::ROR, &a::ABS, 6},
        {"RRA", &a::RRA, &a::ABS, 6}, {"BVS", &a::BVS, &a::REL, 2}, {"ADC", &a::ADC, &a::IZY, 5},
        {"JAM", &a::JAM, &a::IMP, 2}, {"RRA", &a::RRA, &a::IZY, 8}, {"NOP", &a::NOP, &a::ZPX, 4},
        {"ADC", &a::ADC, &a::ZPX, 4}, {"ROR", &a::ROR, &a::ZPX, 6}, {"RRA", &a::RRA, &a::ZPX, 6},
        {"SEI", &a::SEI, &a::IMP, 2}, {"ADC", &a::ADC, &a::ABY, 4}, {"NOP", &a::NOP, &a::IMP, 2},
        {"RRA", &a::RRA, &a::ABY, 7}, {"NOP", &a::NOP, &a::ABX, 4}, {"ADC", &a::ADC, &a::ABX, 4},
        {"ROR", &a::ROR, &a::ABX, 7}, {"RRA", &a::RRA, &a::ABX, 7}, {"NOP", &a::NOP, &a::IMM, 2},
        {"STA", &a::STA, &a::IZX, 6}, {"NOP", &a::NOP, &a::IMM, 2}, {"SAX", &a::SAX, &a::IZX, 6},
        {"STY", &a::STY, &a::ZP0, 3}, {"STA", &a::STA, &a::ZP0, 3}, {"STX", &a::STX, &a::ZP0, 3},
        {"SAX", &a::SAX, &a::ZP0, 3}, {"DEY", &a::DEY, &a::IMP, 2}, {"NOP", &a::NOP, &a::IMM, 2},
        {"TXA", &a::TXA, &a::IMP, 2}, {"ANE", &a::ANE, &a::IMM, 2}, {"STY", &a::STY, &a::ABS, 4},
        {"STA", &a::STA, &a::ABS, 4}, {"STX", &a::STX, &a::ABS, 4}, {"SAX", &a::SAX, &a::ABS, 4},
        {"BCC", &a::BCC, &a::REL, 2}, {"STA", &a::STA, &a::IZY, 6}, {"JAM", &a::JAM, &a::IMP, 2},
        {"SHA", &a::SHA, &a::IZY, 6}, {"STY", &a::STY, &a::ZPX, 4}, {"STA", &a::STA, &a::ZPX, 4},
        {"STX", &a::STX, &a::ZPY, 4}, {"SAX", &a::SAX, &a::ZPY, 4}, {"TYA", &a::TYA, &a::IMP, 2},
        {"STA", &a::STA, &a::ABY, 5}, {"TXS", &a::TXS, &a::IMP, 2}, {"TAS", &a::TAS, &a::ABY, 5},
        {"SHY", &a::SHY, &a::ABX, 5}, {"STA", &a::STA, &a::ABX, 5}, {"SHX", &a::SHX, &a::ABY, 5},
        {"SHA", &a::SHA, &a::ABY, 5}, {"LDY", &a::LDY, &a::IMM, 2}, {"LDA", &a::LDA, &a::IZX, 6},
        {"LDX", &a::LDX, &a::IMM, 2}, {"LAX", &a::LAX, &a::IZX, 6}, {"LDY", &a::LDY, &a::ZP0, 3},
        {"LDA", &a::LDA, &a::ZP0, 3}, {"LDX", &a::LDX, &a::ZP0, 3}, {"LAX", &a::LAX, &a::ZP0, 3},
        {"TAY", &a::TAY, &a::IMP, 2}, {"LDA", &a::LDA, &a::IMM, 2}, {"TAX", &a::TAX, &a::IMP, 2},
        {"LXA", &a::LXA, &a::IMM, 2}, {"LDY", &a::LDY, &a::ABS, 4}, {"LDA", &a::LDA, &a::ABS, 4},
        {"LDX", &a::LDX, &a::ABS, 4}, {"LAX", &a::LAX, &a::ABS, 4}, {"BCS", &a::BCS, &a::REL, 2},
        {"LDA", &a::LDA, &a::IZY, 5}, {"JAM", &a::JAM, &a::IMP, 2}, {"LAX", &a::LAX, &a::IZY, 5},
        {"LDY", &a::LDY, &a::ZPX, 4}, {"LDA", &a::LDA, &a::ZPX, 4}, {"LDX", &a::LDX, &a::ZPY, 4},
        {"LAX", &a::LAX, &a::ZPY, 4}, {"CLV", &a::CLV, &a::IMP, 2}, {"LDA", &a::LDA, &a::ABY, 4},
        {"TSX", &a::TSX, &a::IMP, 2}, {"LAS", &a::LAS, &a::ABY, 4}, {"LDY", &a::LDY, &a::ABX, 4},
        {"LDA", &a::LDA, &a::ABX, 4}, {"LDX", &a::LDX, &a::ABY, 4}, {"LAX", &a::LAX, &a::ABY, 4},
        {"CPY", &a::CPY, &a::IMM, 2}, {"CMP", &a::CMP, &a::IZX, 6}, {"NOP", &a::NOP, &a::IMM, 2},
        {"DCP", &a::DCP, &a::IZX, 8}, {"CPY", &a::CPY, &a::ZP0, 3}, {"CMP", &a::CMP, &a::ZP0, 3},
        {"DEC", &a::DEC, &a::ZP0, 5}, {"DCP", &a::DCP, &a::ZP0, 5}, {"INY", &a::INY, &a::IMP, 2},
        {"CMP", &a::CMP, &a::IMM, 2}, {"DEX", &a::DEX, &a::IMP, 2}, {"SBX", &a::SBX, &a::IMM, 2},
        {"CPY", &a::CPY, &a::ABS, 4}, {"CMP", &a::CMP, &a::ABS, 4}, {"DEC", &a::DEC, &a::ABS, 6},
        {"DCP", &a::DCP, &a::ABS, 6}, {"BNE", &a::BNE, &a::REL, 2}, {"CMP", &a::CMP, &a::IZY, 5},
        {"JAM", &a::JAM, &a::IMP, 2}, {"DCP", &a::DCP, &a::IZY, 8}, {"NOP", &a::NOP, &a::ZPX, 4},
        {"CMP", &a::CMP, &a::ZPX, 4}, {"DEC", &a::DEC, &a::ZPX, 6}, {"DCP", &a::DCP, &a::ZPX, 6},
        {"CLD", &a::CLD, &a::IMP, 2}, {"CMP", &a::CMP, &a::ABY, 4}, {"NOP", &a::NOP, &a::IMP, 2},
        {"DCP", &a::DCP, &a::ABY, 7}, {"NOP", &a::NOP, &a::ABX, 4}, {"CMP", &a::CMP, &a::ABX, 4},
        {"DEC", &a::DEC, &a::ABX, 7}, {"DCP", &a::DCP, &a::ABX, 7}, {"CPX", &a::CPX, &a::IMM, 2},
        {"SBC", &a::SBC, &a::IZX, 6}, {"NOP", &a::NOP, &a::IMM, 2}, {"ISC", &a::ISC, &a::IZX, 8},
        {"CPX", &a::CPX, &a::ZP0, 3}, {"SBC", &a::SBC, &a::ZP0, 3}, {"INC", &a::INC, &a::ZP0, 5},
        {"ISC", &a::ISC, &a::ZP0, 5}, {"INX", &a::INX, &a::IMP, 2}, {"SBC", &a::SBC, &a::IMM, 2},
        {"NOP", &a::NOP, &a::IMP, 2}, {"SBC", &a::SBC, &a::IMM, 2}, {"CPX", &a::CPX, &a::ABS, 4},
        {"SBC", &a::SBC, &a::ABS, 4}, {"INC", &a::INC, &a::ABS, 6}, {"ISC", &a::ISC, &a::ABS, 6},
        {"BEQ", &a::BEQ, &a::REL, 2}, {"SBC", &a::SBC, &a::IZY, 5}, {"JAM", &a::JAM, &a::IMP, 2},
        {"ISC", &a::ISC, &a::IZY, 4}, {"NOP", &a::NOP, &a::ZPX, 4}, {"SBC", &a::SBC, &a::ZPX, 4},
        {"INC", &a::INC, &a::ZPX, 6}, {"ISC", &a::ISC, &a::ZPX, 6}, {"SED", &a::SED, &a::IMP, 2},
        {"SBC", &a::SBC, &a::ABY, 4}, {"NOP", &a::NOP, &a::IMP, 2}, {"ISC", &a::ISC, &a::ABY, 7},
        {"NOP", &a::NOP, &a::ABX, 4}, {"SBC", &a::SBC, &a::ABX, 4}, {"INC", &a::INC, &a::ABX, 7},
        {"ISC", &a::ISC, &a::ABX, 7},
    };
}

void Processor6502::SetPC(uint16_t pc)
{
    // Little verification to catch when PC goes wild
    if (pc < 0x8000)
    {
        // assert(false && "PC went wild");
    }
    m_PC = pc;
}

void Processor6502::Write(uint16_t address, uint8_t data, bool gardingStack)
{
    if (m_isJammed)
        return;

    uint16_t mirroredAddr = address % Cst::RAM_SIZE;
    if (!((mirroredAddr >= STACK_LOCATION + m_SP && mirroredAddr <= 0x01ff) ^ !gardingStack))
    {
        // assert(false && "Accessing the stack directly, forbidden");
    }

    m_bus->WriteCPU(address, data);
}

uint8_t Processor6502::Read(uint16_t address, bool gardingStack)
{
    if (m_isJammed)
        return 0xFF;

    uint16_t mirroredAddr = address % Cst::RAM_SIZE;
    if (!((mirroredAddr >= STACK_LOCATION + m_SP && mirroredAddr <= 0x01ff) ^ !gardingStack))
    {
        // assert(false && "Accessing the stack directly, forbidden");
    }
    return m_bus->ReadCPU(address);
}

void Processor6502::Clock()
{
    if (m_isJammed)
        return;

    if (m_cycles == 0)
    {
        m_status.U = 1;

        m_opcode = Read(m_PC++);
        m_cycles = m_opCodeMapper[m_opcode].cycles;

        // Execute address mode
        uint8_t additional_cyles_addr = (this->*m_opCodeMapper[m_opcode].addrmode)();
        // Execute op
        uint8_t additional_cyles_op = (this->*m_opCodeMapper[m_opcode].operate)();

        m_cycles += (additional_cyles_addr & additional_cyles_op);

#if LOGGING == 1
        std::cout << "Executing op " << m_opCodeMapper[m_opcode].name << std::endl;
#endif

        m_status.U = 1;
    }

    m_opComplete = (--m_cycles == 0);
    m_numberOfCycles++;
}

uint8_t Processor6502::Fetch()
{
    if (m_opCodeMapper[m_opcode].addrmode != &Processor6502::IMP)
        m_fetched = Read(m_absAddress);

    return m_fetched;
}

void Processor6502::Reset()
{
    // Reset the CPU to a known state
    m_absAddress = 0xFFFC;
    uint8_t low = Read(m_absAddress);
    uint8_t high = Read(m_absAddress + 1);

    SetPC((high << 8) | low);

    m_A = 0;
    m_X = 0;
    m_Y = 0;
    m_SP = 0xFD;
    m_fetched = 0x00;

    m_status.flags = 0x00;
    m_status.U = 1;
    m_status.I = 1;

    m_relAddress = 0x0000;
    m_absAddress = 0x0000;

    // Reset takes time
    m_cycles = 7;
    m_opComplete = false;
    m_isJammed = false;
}

void Processor6502::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    visitor.WriteValue(SerializerVersion::Current);

    visitor.WriteValue(m_A);
    visitor.WriteValue(m_X);
    visitor.WriteValue(m_Y);
    visitor.WriteValue(m_SP);
    visitor.WriteValue(m_PC);
    visitor.WriteValue(m_absAddress);
    visitor.WriteValue(m_relAddress);
    visitor.WriteValue(m_fetched);
    visitor.WriteValue(m_opcode);
    visitor.WriteValue(m_cycles);
    visitor.WriteValue(m_status);
    visitor.WriteValue(m_numberOfCycles);
    visitor.WriteValue(m_opComplete);
    visitor.WriteValue(m_isJammed);
}

void Processor6502::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    SerializerVersion version;

    visitor.ReadValue(version);
    assert(version <= SerializerVersion::Current);

    visitor.ReadValue(m_A);
    visitor.ReadValue(m_X);
    visitor.ReadValue(m_Y);
    visitor.ReadValue(m_SP);
    visitor.ReadValue(m_PC);
    visitor.ReadValue(m_absAddress);
    visitor.ReadValue(m_relAddress);
    visitor.ReadValue(m_fetched);
    visitor.ReadValue(m_opcode);
    visitor.ReadValue(m_cycles);
    visitor.ReadValue(m_status);
    visitor.ReadValue(m_numberOfCycles);
    visitor.ReadValue(m_opComplete);
    visitor.ReadValue(m_isJammed);
}

void Processor6502::IRQ()
{
    if (m_status.I == 0)
    {
        Interrupt(0, 0xFFFE);
        m_cycles = 7;
    }
}

void Processor6502::NMI()
{
    Interrupt(0, 0xFFFA);
    m_cycles = 7;
}

/////////////////////////////////////////////
// Addressing modes
/////////////////////////////////////////////
uint8_t Processor6502::IMP()
{
    m_fetched = m_A;
    return 0;
}

uint8_t Processor6502::IMM()
{
    m_absAddress = m_PC++;
    return 0;
}

uint8_t Processor6502::ZP0()
{
    m_absAddress = Read(m_PC++);
    m_absAddress &= 0x00FF;
    return 0;
}

uint8_t Processor6502::ZPX()
{
    m_absAddress = Read(m_PC++) + m_X;
    m_absAddress &= 0x00FF;
    return 0;
}

uint8_t Processor6502::ZPY()
{
    m_absAddress = Read(m_PC++) + m_Y;
    m_absAddress &= 0x00FF;
    return 0;
}

uint8_t Processor6502::REL()
{
    m_relAddress = Read(m_PC++);
    // If the highest bit of the first byte is set, it means it is a negative one
    if (m_relAddress & 0x80)
        m_relAddress |= 0xFF00;

    return 0;
}

uint8_t Processor6502::ABS()
{
    uint16_t low = Read(m_PC++);
    uint16_t high = Read(m_PC++);

    m_absAddress = (high << 8) | low;

    return 0;
}

uint8_t Processor6502::ABX()
{
    uint16_t low = Read(m_PC++);
    uint16_t high = Read(m_PC++);

    m_absAddress = ((high << 8) | low) + m_X;

    // If the high bytes are different, it means we changed pages.
    // It introduces an extra cycle.
    if ((m_absAddress & 0xFF00) != (high << 8))
        return 1;

    return 0;
}

uint8_t Processor6502::ABY()
{
    uint16_t low = Read(m_PC++);
    uint16_t high = Read(m_PC++);

    m_absAddress = ((high << 8) | low) + m_Y;

    // If the high bytes are different, it means we changed pages.
    // It introduces an extra cycle.
    if ((m_absAddress & 0xFF00) != (high << 8))
        return 1;

    return 0;
}

uint8_t Processor6502::IND()
{
    uint16_t lowPtr = Read(m_PC++);
    uint16_t highPtr = Read(m_PC++);

    uint16_t ptr = ((highPtr << 8) | lowPtr);

    uint16_t low = Read(ptr);

    // cf. https://wiki.nesdev.org/w/index.php?title=CPU_addressing_modes
    // if lowPtr == 255, lowPtr + 1 should be 256 (0x0100) and highPtr should be incremented
    // by one. But in reality, we stay on the same page. so we read the first byte at 0x(highPtr)FF
    // and read the second byte at 0x(highPtr)00.
    ptr = lowPtr == 0xFF ? (highPtr << 8) : ptr + 1;
    uint16_t high = Read(ptr);

    m_absAddress = ((high << 8) | low);
    return 0;
}

uint8_t Processor6502::IZX()
{
    uint16_t ptr = Read(m_PC++);

    uint16_t low = Read((ptr + m_X) & 0x00FF);
    uint16_t high = Read((ptr + m_X + 1) & 0x00FF);

    m_absAddress = ((high << 8) | low);
    return 0;
}

uint8_t Processor6502::IZY()
{
    uint16_t ptr = Read(m_PC++);

    uint16_t low = Read(ptr & 0x00FF);
    uint16_t high = Read((ptr + 1) & 0x00FF);

    m_absAddress = ((high << 8) | low) + m_Y;

    // If the high bytes are different, it means we changed pages.
    // It introduces an extra cycle.
    if ((m_absAddress & 0xFF00) != (high << 8))
        return 1;

    return 0;
}

////////////////////////////////////////////////
// Helpers
////////////////////////////////////////////////

uint8_t Processor6502::Branch(uint8_t flag, uint8_t isSet)
{
    if (flag == isSet)
    {
        m_cycles++;
        m_absAddress = m_PC + m_relAddress;

        if ((m_absAddress & 0xFF00) != (m_PC & 0xFF00))
            m_cycles++;

        SetPC(m_absAddress);
    }

    return 0;
}

inline void Processor6502::SetFlagIfNegOrZero(uint8_t value)
{
    m_status.Z = (uint8_t)(value == 0);
    m_status.N = (value & 0x80) >> 7;
}

uint8_t Processor6502::GeneralAddition()
{
    uint16_t temp = (uint16_t)m_A + (uint16_t)m_fetched + (uint16_t)m_status.C;

    // Overflow check
    // Overflow if the sign of accumulator and memory are the same
    // but the sign of the accumulator and the result is different.
    uint8_t signA = (m_A & 0x80) >> 7;
    uint8_t signFetched = (m_fetched & 0x80) >> 7;
    uint8_t signR = ((uint8_t)temp & 0x80) >> 7;

    // m_status.V = (signA ^ signR) & ~(signA ^ signFetched);
    m_status.V = ((~((uint16_t)m_A ^ (uint16_t)m_fetched) & ((uint16_t)m_A ^ (uint16_t)temp)) & 0x0080) > 0;

    // Carry check
    m_status.C = temp > 255u;

    m_A = (uint8_t)(temp & 0x00FF);

    SetFlagIfNegOrZero(m_A);

    return 1;
}

void Processor6502::PushDataToStack(uint8_t data)
{
    Write(STACK_LOCATION + m_SP, data, true);
    m_SP--;
}

void Processor6502::PushAddrToStack(uint16_t addr)
{
    // First push high, then low
    Write(STACK_LOCATION + m_SP, (uint8_t)((addr >> 8) & 0x00FF), true);
    m_SP--;

    Write(STACK_LOCATION + m_SP, (uint8_t)(addr & 0x00FF), true);
    m_SP--;
}

uint8_t Processor6502::PopDataFromStack()
{
    m_SP++;
    return Read(STACK_LOCATION + m_SP, true);
}

uint16_t Processor6502::PopAddrFromStack()
{
    m_SP++;
    uint8_t low = Read(STACK_LOCATION + m_SP, true);

    m_SP++;
    uint8_t high = Read(STACK_LOCATION + m_SP, true);

    return (high << 8) | low;
}

void Processor6502::Interrupt(uint8_t requestSoftware, uint16_t jumpAddressLocation)
{
    // Write the PC to the stack.
    PushAddrToStack(m_PC);

    // Set the internal flags.
    // For the B flag, it should be set to 0 if the interrupt comes from hardware
    // and 1 if it comes from software
    m_status.B = requestSoftware;
    m_status.U = 1;

    // Then push the status flag to the stack
    PushDataToStack(m_status.flags);

    m_status.I = 1;

    // Re-set the B status to 0
    m_status.B = 0;

    // And finally jump to a known location
    uint8_t low = Read(jumpAddressLocation);
    uint8_t high = Read(jumpAddressLocation + 1);
    SetPC((high << 8) | low);
}

////////////////////////////////////////////////
// Opcodes
////////////////////////////////////////////////

uint8_t Processor6502::ADC()
{
    Fetch();
    return GeneralAddition();
}

uint8_t Processor6502::AND()
{
    Fetch();

    m_A &= m_fetched;

    SetFlagIfNegOrZero(m_A);

    return 1;
}

uint8_t Processor6502::ASL()
{
    Fetch();

    uint8_t temp = m_fetched << 1;
    m_status.C = (m_fetched & 0x80) >> 7;

    SetFlagIfNegOrZero(temp);

    // Depending on the addr mode, we need to write this value to the accumulator or in memory
    if (m_opCodeMapper[m_opcode].addrmode == &Processor6502::IMP)
        m_A = temp;
    else
        Write(m_absAddress, temp);

    return 0;
}

uint8_t Processor6502::BCC() { return Branch(m_status.C, 0); }

uint8_t Processor6502::BCS() { return Branch(m_status.C, 1); }

uint8_t Processor6502::BEQ() { return Branch(m_status.Z, 1); }

uint8_t Processor6502::BIT()
{
    Fetch();

    uint8_t temp = m_A & m_fetched;

    m_status.Z = (temp == 0x00);
    m_status.N = (m_fetched & 0x80) >> 7;
    m_status.V = (m_fetched & 0x40) >> 6;

    return 0;
}

uint8_t Processor6502::BMI() { return Branch(m_status.N, 1); }

uint8_t Processor6502::BNE() { return Branch(m_status.Z, 0); }

uint8_t Processor6502::BPL() { return Branch(m_status.N, 0); }

uint8_t Processor6502::BRK()
{
    // We need to push PC+2 because there is a spacing where we can provide
    // a reason for the break.
    // Because we set the BRK address mode to Immediate, the PC is already
    // incremented by one. We can read the reason with "Fetch", it will be in m_fetched afterwards.
    // So we only increment by one here.
    m_PC++;
    Interrupt(1, 0xFFFE);
    return 0;
}

uint8_t Processor6502::BVC() { return Branch(m_status.V, 0); }

uint8_t Processor6502::BVS() { return Branch(m_status.V, 1); }

uint8_t Processor6502::CLC()
{
    m_status.C = 0;
    return 0;
}

uint8_t Processor6502::CLD()
{
    m_status.D = 0;
    return 0;
}

uint8_t Processor6502::CLI()
{
    m_status.I = 0;
    return 0;
}

uint8_t Processor6502::CLV()
{
    m_status.V = 0;
    return 0;
}

uint8_t Processor6502::CMP()
{
    Fetch();

    uint8_t temp = m_A - m_fetched;

    SetFlagIfNegOrZero(temp);
    m_status.C = m_A >= m_fetched;

    return 1;
}

uint8_t Processor6502::CPX()
{
    Fetch();

    uint8_t temp = m_X - m_fetched;

    SetFlagIfNegOrZero(temp);
    m_status.C = m_X >= m_fetched;

    return 0;
}

uint8_t Processor6502::CPY()
{
    Fetch();

    uint8_t temp = m_Y - m_fetched;

    SetFlagIfNegOrZero(temp);
    m_status.C = m_Y >= m_fetched;

    return 0;
}

uint8_t Processor6502::DEC()
{
    Fetch();

    uint8_t temp = m_fetched - 1;
    Write(m_absAddress, temp);

    SetFlagIfNegOrZero(temp);

    return 0;
}

uint8_t Processor6502::DEX()
{
    m_X--;
    SetFlagIfNegOrZero(m_X);
    return 0;
}

uint8_t Processor6502::DEY()
{
    m_Y--;
    SetFlagIfNegOrZero(m_Y);
    return 0;
}

uint8_t Processor6502::EOR()
{
    Fetch();

    m_A ^= m_fetched;

    SetFlagIfNegOrZero(m_A);

    return 1;
}

uint8_t Processor6502::INC()
{
    Fetch();

    uint8_t temp = m_fetched + 1;
    Write(m_absAddress, temp);

    SetFlagIfNegOrZero(temp);

    return 1;
}

uint8_t Processor6502::INX()
{
    m_X++;
    SetFlagIfNegOrZero(m_X);
    return 0;
}

uint8_t Processor6502::INY()
{
    m_Y++;
    SetFlagIfNegOrZero(m_Y);
    return 0;
}

uint8_t Processor6502::JMP()
{
    SetPC(m_absAddress);
    return 0;
}

uint8_t Processor6502::JSR()
{
    // We need to push PC + 2
    // Since our mode is absolute, it means we already incremented PC
    // by 3 (opcode + address = 3 bytes)
    // So decrement it.
    m_PC--;

    PushAddrToStack(m_PC);
    return JMP();
}

uint8_t Processor6502::LDA()
{
    Fetch();

    m_A = m_fetched;
    SetFlagIfNegOrZero(m_A);

    return 1;
}

uint8_t Processor6502::LDX()
{
    Fetch();

    m_X = m_fetched;
    SetFlagIfNegOrZero(m_X);

    return 1;
}

uint8_t Processor6502::LDY()
{
    Fetch();

    m_Y = m_fetched;
    SetFlagIfNegOrZero(m_Y);

    return 1;
}

uint8_t Processor6502::LSR()
{
    Fetch();

    m_status.C = m_fetched & 0x01;

    uint8_t temp = m_fetched >> 1;

    SetFlagIfNegOrZero(temp);

    // Depending on the addr mode, we need to write this value to the accumulator or in memory
    if (m_opCodeMapper[m_opcode].addrmode == &Processor6502::IMP)
        m_A = temp;
    else
        Write(m_absAddress, temp);

    return 0;
}

uint8_t Processor6502::NOP()
{
    // Not all NOPs are equal.
    switch (m_opcode)
    {
    case 0x1C:
    case 0x3C:
    case 0x5C:
    case 0x7C:
    case 0xDC:
    case 0xFC:
        return 1;
        break;
    }
    return 0;
}

uint8_t Processor6502::ORA()
{
    Fetch();

    m_A |= m_fetched;
    SetFlagIfNegOrZero(m_A);

    return 1;
}

uint8_t Processor6502::PHA()
{
    PushDataToStack(m_A);
    return 0;
}

uint8_t Processor6502::PHP()
{
    uint8_t savedB = m_status.B;
    uint8_t savedU = m_status.U;
    m_status.B = 1;
    m_status.U = 1;
    PushDataToStack(m_status.flags);
    m_status.B = savedB;
    m_status.U = savedU;
    return 0;
}

uint8_t Processor6502::PLA()
{
    m_A = PopDataFromStack();
    SetFlagIfNegOrZero(m_A);
    return 0;
}

uint8_t Processor6502::PLP()
{
    uint8_t savedU = m_status.U;
    uint8_t savedB = m_status.B;
    m_status.flags = PopDataFromStack();
    m_status.U = savedU;
    m_status.B = savedB;
    return 0;
}

uint8_t Processor6502::ROL()
{
    Fetch();

    uint8_t temp = (m_fetched << 1) | m_status.C;

    m_status.C = (m_fetched & 0x80) >> 7;

    SetFlagIfNegOrZero(temp);

    // Depending on the addr mode, we need to write this value to the accumulator or in memory
    if (m_opCodeMapper[m_opcode].addrmode == &Processor6502::IMP)
        m_A = temp;
    else
        Write(m_absAddress, temp);

    return 0;
}

uint8_t Processor6502::ROR()
{
    Fetch();

    uint8_t temp = (m_fetched >> 1) | (m_status.C << 7);

    m_status.C = m_fetched & 0x01;

    SetFlagIfNegOrZero(temp);

    // Depending on the addr mode, we need to write this value to the accumulator or in memory
    if (m_opCodeMapper[m_opcode].addrmode == &Processor6502::IMP)
        m_A = temp;
    else
        Write(m_absAddress, temp);

    return 0;
}

uint8_t Processor6502::RTI()
{
    uint8_t savedU = m_status.U;
    m_status.flags = PopDataFromStack();
    m_status.U = savedU;

    SetPC(PopAddrFromStack());
    return 0;
}

uint8_t Processor6502::RTS()
{
    SetPC(PopAddrFromStack());
    m_PC++;
    return 0;
}

uint8_t Processor6502::SBC()
{
    // Substraction is an addition with -fetched
    Fetch();
    m_fetched = ~(m_fetched);

    return GeneralAddition();
}

uint8_t Processor6502::SEC()
{
    m_status.C = 1;
    return 0;
}

uint8_t Processor6502::SED()
{
    m_status.D = 1;
    return 0;
}

uint8_t Processor6502::SEI()
{
    m_status.I = 1;
    return 0;
}

uint8_t Processor6502::STA()
{
    Write(m_absAddress, m_A);
    return 0;
}

uint8_t Processor6502::STX()
{
    Write(m_absAddress, m_X);
    return 0;
}

uint8_t Processor6502::STY()
{
    Write(m_absAddress, m_Y);
    return 0;
}

uint8_t Processor6502::TAX()
{
    m_X = m_A;
    SetFlagIfNegOrZero(m_X);
    return 0;
}

uint8_t Processor6502::TAY()
{
    m_Y = m_A;
    SetFlagIfNegOrZero(m_Y);
    return 0;
}

uint8_t Processor6502::TSX()
{
    m_X = m_SP;
    SetFlagIfNegOrZero(m_X);
    return 0;
}

uint8_t Processor6502::TXA()
{
    m_A = m_X;
    SetFlagIfNegOrZero(m_A);
    return 0;
}

uint8_t Processor6502::TXS()
{
    m_SP = m_X;
    return 0;
}

uint8_t Processor6502::TYA()
{
    m_A = m_Y;
    SetFlagIfNegOrZero(m_A);
    return 0;
}

///////////// Illegal opcodes //////////////

uint8_t Processor6502::JAM()
{
    m_isJammed = true;
    return 0;
}

uint8_t Processor6502::ALR()
{
    AND();
    LSR();
    return 0;
}

uint8_t Processor6502::ANC()
{
    AND();
    m_status.C = (m_fetched & 0x80) >> 7;
    return 0;
}

uint8_t Processor6502::ANE()
{
    // * AND X + AND oper

    // Highly unstable, do not use.
    // A base value in A is determined based on the content
    // of A and a constant, which may be typically $00,
    // $ff, $ee, etc. The value of this constant depends on
    // temerature, the chip series, and maybe other factors, as well.
    // In order to eliminate these uncertaincies from the equation,
    // use either 0 as the operand or a value of $FF in the accumulator.

    // (A OR CONST) AND X AND oper -> A
    return 0;
}

uint8_t Processor6502::ARR() { return 0; }

uint8_t Processor6502::DCP()
{
    DEC();
    CMP();
    return 0;
}

uint8_t Processor6502::ISC()
{
    INC();
    SBC();
    return 0;
}

uint8_t Processor6502::LAS()
{
    Fetch();

    uint8_t temp = m_fetched & m_SP;
    m_A = temp;
    m_X = temp;
    m_SP = temp;
    return 1;
}

uint8_t Processor6502::LAX()
{
    LDA();
    LDX();
    return 1;
}

uint8_t Processor6502::LXA()
{
    // Store * AND oper in A and X
    // Highly unstable, involves a 'magic' constant, see ANE
    // (A OR CONST) AND oper -> A -> X
    return 0;
}

uint8_t Processor6502::RLA()
{
    ROL();
    AND();
    return 0;
}

uint8_t Processor6502::RRA()
{
    ROR();
    ADC();
    return 0;
}

uint8_t Processor6502::SAX()
{
    Write(m_absAddress, m_A & m_X);
    return 0;
}

uint8_t Processor6502::SBX() { return 0; }

uint8_t Processor6502::SHA()
{
    // Stores A AND X AND (high-byte of addr. + 1) at addr.
    // unstable: sometimes 'AND (H+1)' is dropped, page boundary
    // crossings may not work (with the high-byte of the value used
    // as the high-byte of the address)
    // A AND X AND (H+1) -> M
    return 0;
}

uint8_t Processor6502::SHX()
{
    // Stores X AND (high-byte of addr. + 1) at addr.
    // unstable: sometimes 'AND (H+1)' is dropped,
    // page boundary crossings may not work (with the
    // high-byte of the value used as the high-byte of the address)
    // X AND (H+1) -> M
    return 0;
}

uint8_t Processor6502::SHY()
{
    // Stores Y AND (high-byte of addr. + 1) at addr.
    // unstable: sometimes 'AND (H+1)' is dropped,
    // page boundary crossings may not work (with the
    // high-byte of the value used as the high-byte of the address)
    // Y AND (H+1) -> M
    return 0;
}

uint8_t Processor6502::SLO()
{
    ASL();
    ORA();
    return 0;
}

uint8_t Processor6502::SRE()
{
    LSR();
    EOR();
    return 0;
}

uint8_t Processor6502::TAS()
{
    // Puts A AND X in SP and stores A AND X AND (high-byte of addr. + 1) at addr.
    // unstable: sometimes 'AND (H+1)' is dropped, page boundary crossings
    // may not work (with the high-byte of the value used as the high-byte
    // of the address)
    // A AND X -> SP, A AND X AND (H+1) -> M
    return 0;
}
