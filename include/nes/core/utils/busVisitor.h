#pragma once

#include <common/utils/visitor.h>
#include <cstdint>

namespace NesEmulator
{
class Bus;

namespace Utils
{
class BusReadVisitor : public Common::Utils::IReadVisitor
{
public:
    BusReadVisitor(const Bus& bus, uint16_t startAddr, uint16_t endAddr);
    void Read(uint8_t* data, size_t size) override;
    void Peek(uint8_t* data, size_t size) override;

    using IReadVisitor::Peek;
    using IReadVisitor::Read;
    void Advance(size_t size) override;
    size_t Remaining() const override;

    uint16_t GetCurrentPtr() const { return m_ptr; }

private:
    const Bus& m_bus;
    uint16_t m_ptr;
    uint16_t m_endAddr;
};
} // namespace Utils
} // namespace NesEmulator