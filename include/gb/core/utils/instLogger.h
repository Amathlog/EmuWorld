#pragma once

#include <common/utils/visitor.h>
#include <memory>
#include <string>

namespace GBEmulator
{
class Bus;

class InstructionLogger
{
public:
    InstructionLogger(const GBEmulator::Bus& bus)
        : m_bus(bus)
    {
    }

    void OpenFileVisitor(const std::string& filepath);

    void WriteCurrentState();
    void Reset();

private:
    const GBEmulator::Bus& m_bus;
    std::unique_ptr<Common::Utils::IWriteVisitor> m_visitor;
};
} // namespace GBEmulator