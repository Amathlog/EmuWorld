#include <core/bus.h>
#include <core/controller.h>
#include <core/utils/busVisitor.h>
#include <core/utils/disassembly.h>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>

#include <core/cartridge.h>
#include <core/constants.h>
#include <string>

using NesEmulator::Bus;
using NesEmulator::Cartridge;
using NesEmulator::Controller;

Bus::Bus()
{
    // Allocate RAM and set it to 0
    m_cpuRam.resize(Cst::RAM_SIZE); // 2KB
    for (auto& v : m_cpuRam)
        v = 0x00;

    // Connect CPU and APU to this bus
    m_cpu.ConnectBus(this);
    m_apu.ConnectBus(this);

    // Default mode
    SetMode(Mode::NTSC);

    m_enabled = true;

    m_audioTimePerSystemSample = 1.0 / NesEmulator::Cst::SAMPLE_RATE;
}

void Bus::WriteCPU(uint16_t address, uint8_t data)
{
    if (m_cartridge->WriteCPU(address, data))
    {
        // Do nothing
    }
    else if (address >= Cst::RAM_START_ADDR && address <= Cst::RAM_END_ADDR)
    {
        // Mirroring. Map to 0x0000 to 0x0800
        m_cpuRam[address % Cst::RAM_SIZE] = data;
    }
    else if (address >= Cst::PPU_REG_START_ADDR && address <= Cst::PPU_REG_END_ADDR)
    {
        // Mirroring.
        m_ppu.WriteCPU(address % Cst::PPU_REG_SIZE, data);
    }
    else if (address == Cst::DMA_REGISTER_ADDR)
    {
        m_dmaPage = data;
        m_dmaAddr = 0x00;
        m_dmaTransfer = true;
        m_dmaWaitForCPU = true;
    }
    else if (address == Cst::CONTROLLER_1_ADDR)
    {
        // When writing to this register, pull both controllers
        for (auto index = 0; index < m_controllers.size(); ++index)
        {
            if (m_controllers[index].get() != nullptr)
                m_controllersState[index] = m_controllers[index]->GetButtonsState();
        }
    }
    else if (address >= Cst::APU_IO_START_ADDR && address <= Cst::APU_IO_END_ADDR)
    {
        m_apu.WriteCPU(address, data);
    }
}

uint8_t Bus::ReadCPU(uint16_t address)
{
    uint8_t data = 0;
    if (m_cartridge->ReadCPU(address, data))
    {
        // Done
    }
    else if (address >= Cst::RAM_START_ADDR && address <= Cst::RAM_END_ADDR)
    {
        // Mirroring. Map to 0x0000 to 0x07FF
        data = m_cpuRam[address % Cst::RAM_SIZE];
    }
    else if (address >= Cst::PPU_REG_START_ADDR && address <= Cst::PPU_REG_END_ADDR)
    {
        // Mirroring.
        data = m_ppu.ReadCPU(address % Cst::PPU_REG_SIZE);
    }
    else if (address == Cst::CONTROLLER_1_ADDR || address == Cst::CONTROLLER_2_ADDR)
    {
        uint16_t index = address & 0x0001;
        data = (m_controllersState[index] & 0x80) > 0;
        m_controllersState[index] <<= 1;
    }
    else if (address >= Cst::APU_IO_START_ADDR && address <= Cst::APU_IO_END_ADDR)
    {
        data = m_apu.ReadCPU(address);
    }

    return data;
}

void Bus::Verbose()
{
    if (!m_cpu.IsOpComplete())
        return;

    auto addPadding = [](size_t p)
    {
        for (size_t i = 0; i < p; ++i)
        {
            std::cout << " ";
        }
    };

    Utils::BusReadVisitor visitor(*this, m_cpu.GetPC(), m_cpu.GetPC() + 3);
    uint16_t dummy;
    auto line = NesEmulator::Utils::Disassemble(visitor, m_cpu.GetPC(), dummy);
    std::cout << line[0];
    // Padding
    size_t padding = 30;
    padding -= line[0].size();
    addPadding(padding);
    std::cout << " A:" << std::hex << +m_cpu.GetA();
    std::cout << " X:" << std::hex << +m_cpu.GetX();
    std::cout << " Y:" << std::hex << +m_cpu.GetY();
    std::cout << " P:" << std::hex << +m_cpu.GetStatus().flags;
    std::cout << " SP:" << std::hex << +m_cpu.GetSP();
    int16_t scanlines = m_ppu.GetScanlines();
    int16_t cycles = m_ppu.GetCycles();
    padding = scanlines >= 100 ? 0 : ((scanlines >= 10 || scanlines == -1) ? 1 : 2);
    std::cout << " PPU:";
    addPadding(padding);
    std::cout << std::dec << scanlines << ",";
    padding = cycles >= 100 ? 0 : ((cycles >= 10 || cycles == -1) ? 1 : 2);
    addPadding(padding);
    std::cout << cycles;
    std::cout << " CYC:" << m_cpu.GetNbOfTotalCycles() << std::endl;
}

bool Bus::Clock()
{
    if (!m_enabled || !m_cartridge)
        return false;

    std::unique_lock<std::mutex> lk(m_lock);

    constexpr bool verbose = false;

    // PPU runs 3 times faster than the CPU
    bool frameComplete = false;
    for (uint8_t i = 0; i < 3; ++i)
    {
        m_ppu.Clock();
        frameComplete |= m_ppu.IsFrameComplete();
    }

    bool evenClockCounter = (m_clockCounter & 0x1) == 0;
    // There is 1 APU cycle every 2 CPU cycles.
    if (evenClockCounter)
    {
        m_apu.Clock();
    }

    if constexpr (verbose)
    {
        Verbose();
    }

    // DMA specific
    // When DMA transfer is enabled, CPU gets suspended
    if (m_dmaTransfer)
    {
        if (m_dmaWaitForCPU)
        {
            // But at the beginning, we need to wait until clock count is odd
            // It can takes 1 or 2 cycles
            if (!evenClockCounter)
                m_dmaWaitForCPU = false;
        }
        else
        {
            // On even cycles, we read the data
            // On odd cycles, we write the data
            if (evenClockCounter)
            {
                uint16_t addr = ((uint16_t)m_dmaPage << 8) | m_dmaAddr;
                m_dmaData = ReadCPU(addr);
            }
            else
            {
                // First write the address for OAM
                WriteCPU(Cst::PPU_REG_START_ADDR + 0x0003, m_dmaAddr);
                // Then write the data
                WriteCPU(Cst::PPU_REG_START_ADDR + 0x0004, m_dmaData);

                // When we reach the last address, we stop
                if (++m_dmaAddr == 0)
                    m_dmaTransfer = false;
            }
        }
    }
    else
    {
        m_cpu.Clock();
        m_cartridge->GetMapper()->CPUClock();
    }

    if (m_cartridge->GetMapper()->ShouldIRQ())
    {
        m_cartridge->GetMapper()->ClearIRQ();
        m_cpu.IRQ();
    }

    if (m_apu.ShouldIRQ())
    {
        m_cpu.IRQ();
    }

    if (m_ppu.IsNMISet())
    {
        m_ppu.ResetNMI();
        m_cpu.NMI();
    }

    // Synchronizing with audio
    m_audioTime += m_audioTimePerCPUClock;
    if (m_audioTime >= m_audioTimePerSystemSample)
    {
        m_apu.SampleRequested();
        m_audioTime -= m_audioTimePerSystemSample;
    }

    m_clockCounter++;

    return frameComplete;
}

void Bus::Reset()
{
    bool shouldResume = IsEnabled();
    Stop();

    m_clockCounter = 0;
    m_audioTime = 0.0;
    m_cpu.Reset();
    m_ppu.Reset();
    m_apu.Reset();
    m_controllersState[0] = 0x00;
    m_controllersState[1] = 0x00;
    if (m_cartridge)
        m_cartridge->Reset();

    if (shouldResume)
        Resume();
}

void Bus::InsertCartridge(const std::shared_ptr<Cartridge>& cartridge)
{
    bool shouldResume = IsEnabled();
    Stop();

    m_cartridge = cartridge;
    m_ppu.ConnectCartridge(cartridge);
    Reset();
    SetMode(m_cartridge->GetMapper()->GetMode());

    if (shouldResume)
        Resume();
}

void Bus::ConnectController(const std::shared_ptr<Controller>& controller, uint8_t controllerIndex)
{
    m_controllers[controllerIndex & 0x01] = controller;
}

void Bus::DisconnectController(uint8_t controllerIndex)
{
    m_controllers[controllerIndex & 0x01].reset();
    m_controllersState[controllerIndex & 0x01] = 0;
}

void Bus::SerializeTo(Common::Utils::IWriteVisitor& visitor) const
{
    std::unique_lock<std::mutex> lk(m_lock);

    // If there is no cartridge loaded, do nothing
    if (m_cartridge.get() == nullptr)
        return;

    // Compute SHA-1 to link this state save to a given game
    std::string hash = m_cartridge->GetSHA1();
    visitor.WriteValue(hash.size());
    visitor.Write(hash.c_str(), hash.size());

    m_cpu.SerializeTo(visitor);
    m_ppu.SerializeTo(visitor);
    m_apu.SerializeTo(visitor);

    visitor.WriteContainer(m_cpuRam);
    m_cartridge->SerializeTo(visitor);

    visitor.WriteValue(m_clockCounter);

    if (m_controllers[0].get() != nullptr)
    {
        visitor.WriteValue((uint8_t)1);
        m_controllers[0]->SerializeTo(visitor);
    }
    else
    {
        visitor.WriteValue((uint8_t)0);
    }

    if (m_controllers[1].get() != nullptr)
    {
        visitor.WriteValue((uint8_t)1);
        m_controllers[1]->SerializeTo(visitor);
    }
    else
    {
        visitor.WriteValue((uint8_t)0);
    }

    visitor.Write(m_controllersState.data(), m_controllersState.size());

    visitor.WriteValue(m_dmaPage);
    visitor.WriteValue(m_dmaAddr);
    visitor.WriteValue(m_dmaData);

    visitor.WriteValue(m_dmaTransfer);
    visitor.WriteValue(m_dmaWaitForCPU);
}

void Bus::DeserializeFrom(Common::Utils::IReadVisitor& visitor)
{
    std::unique_lock<std::mutex> lk(m_lock);

    // If there is no cartridge loaded, do nothing
    if (m_cartridge.get() == nullptr)
    {
        std::cerr << "No game loaded, can't load the save state" << std::endl;
        return;
    }

    // Get SHA-1 to verify this state save is linked to the right game
    std::string hash = m_cartridge->GetSHA1();
    std::string readHash;
    size_t hashSize;
    visitor.ReadValue(hashSize);
    if (hashSize != hash.size())
    {
        std::cerr << "Hashes for the loaded game and the save state are not the same size" << std::endl;
        std::cerr << "Save state: " << hashSize << " Game: " << hash.size() << std::endl;
        return;
    }

    readHash.resize(hashSize);
    visitor.Read(readHash.data(), readHash.size());

    if (readHash != hash)
    {
        std::cerr << "Hashes for the loaded game and the save state are not the same" << std::endl;
        std::cerr << "Save state: " << readHash << " Game: " << hash << std::endl;
        return;
    }

    m_cpu.DeserializeFrom(visitor);
    m_ppu.DeserializeFrom(visitor);
    m_apu.DeserializeFrom(visitor);

    visitor.ReadContainer(m_cpuRam);
    m_cartridge->DeserializeFrom(visitor);

    visitor.ReadValue(m_clockCounter);

    uint8_t controller1Connected;
    visitor.ReadValue(controller1Connected);
    if (controller1Connected)
    {
        m_controllers[0]->DeserializeFrom(visitor);
    }

    uint8_t controller2Connected;
    visitor.ReadValue(controller2Connected);
    if (controller2Connected)
    {
        m_controllers[1]->DeserializeFrom(visitor);
    }

    visitor.Read(m_controllersState.data(), m_controllersState.size());

    visitor.ReadValue(m_dmaPage);
    visitor.ReadValue(m_dmaAddr);
    visitor.ReadValue(m_dmaData);

    visitor.ReadValue(m_dmaTransfer);
    visitor.ReadValue(m_dmaWaitForCPU);
}

void Bus::SaveRAM(Common::Utils::IWriteVisitor& visitor) const
{
    std::unique_lock<std::mutex> lk(m_lock);

    // If there is no cartridge loaded, do nothing
    if (m_cartridge.get() == nullptr)
        return;

    // If there is no persistent memory, do nothing
    if (!m_cartridge->GetMapper()->HasPersistantMemory())
        return;

    // Compute SHA-1 to link this state save to a given game
    std::string hash = m_cartridge->GetSHA1();
    visitor.WriteValue(hash.size());
    visitor.Write(hash.c_str(), hash.size());

    m_cartridge->SaveRAM(visitor);
}

void Bus::LoadRAM(Common::Utils::IReadVisitor& visitor)
{
    std::unique_lock<std::mutex> lk(m_lock);

    // If there is no cartridge loaded, do nothing
    if (m_cartridge.get() == nullptr)
    {
        std::cerr << "No game loaded, can't load the RAM" << std::endl;
        return;
    }

    // If there is no persistent memory, do nothing
    if (!m_cartridge->GetMapper()->HasPersistantMemory())
        return;

    // Get SHA-1 to verify this state save is linked to the right game
    std::string hash = m_cartridge->GetSHA1();
    std::string readHash;
    size_t hashSize;
    visitor.ReadValue(hashSize);
    if (hashSize != hash.size())
    {
        std::cerr << "Hashes for the loaded game and the RAM saved are not the same size" << std::endl;
        std::cerr << "RAM saved: " << hashSize << " Game: " << hash.size() << std::endl;
        return;
    }

    readHash.resize(hashSize);
    visitor.Read(readHash.data(), readHash.size());

    if (readHash != hash)
    {
        std::cerr << "Hashes for the loaded game and the RAM saved are not the same" << std::endl;
        std::cerr << "RAM saved: " << readHash << " Game: " << hash << std::endl;
        return;
    }

    m_cartridge->LoadRAM(visitor);
}

void Bus::SetMode(Mode mode)
{
    bool shouldResume = IsEnabled();
    Stop();

    m_mode = mode;
    m_apu.SetMode(mode);
    m_audioTimePerCPUClock = 1.0 / (mode == Mode::NTSC ? Cst::NTSC_CPU_FREQUENCY : Cst::PAL_CPU_FREQUENCY);

    if (shouldResume)
        Resume();
}