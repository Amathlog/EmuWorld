#include <core/utils/busVisitor.h>
#include <core/bus.h>

using NesEmulator::Utils::BusReadVisitor;
using NesEmulator::Bus;

BusReadVisitor::BusReadVisitor(const Bus& bus, uint16_t startAddr, uint16_t endAddr)
    : m_bus(bus)
    , m_ptr(startAddr)
    , m_endAddr(endAddr)
{

}

void BusReadVisitor::Read(uint8_t* data, size_t size)
{
    Peek(data, size);
    m_ptr += (uint16_t)size;
}

void BusReadVisitor::Peek(uint8_t* data, size_t size)
{
    if (size > Remaining())
        return;

    for (uint16_t i = 0; i < (uint16_t)size; ++i)
    {
        // TO REVIEW
        data[i] = const_cast<Bus&>(m_bus).ReadCPU(m_ptr + i);
    }
}

void BusReadVisitor::Advance(size_t size)
{
    if (size > Remaining())
        return;

    m_ptr += (uint16_t)size;
}

size_t BusReadVisitor::Remaining() const
{
    if (m_ptr <= m_endAddr)
        return m_endAddr - m_ptr;

    return (uint16_t)(-1) - m_ptr + m_endAddr;
}