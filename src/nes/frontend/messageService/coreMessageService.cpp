#include <common/utils/fileVisitor.h>
#include <common/utils/vectorVisitor.h>
#include <core/bus.h>
#include <core/cartridge.h>
#include <filesystem>
#include <frontend/messageService/coreMessageService.h>
#include <frontend/messageService/message.h>
#include <frontend/messageService/messages/audioPayload.h>
#include <frontend/messageService/messages/coreMessage.h>
#include <frontend/messageService/messages/corePayload.h>
#include <frontend/messageService/messages/debugPayload.h>
#include <frontend/utils.h>
#include <iostream>

namespace fs = std::filesystem;

using NesEmulatorGL::CoreMessageService;

bool CreateFolders(const std::string& file)
{
    if (file.empty())
        return true;

    auto parent = fs::path(file).parent_path();
    if (!fs::exists(parent))
    {
        if (!fs::create_directories(parent))
        {
            std::cerr << "Couldn't create the folders for file " << file << std::endl;
            return false;
        }
    }

    return true;
}

bool CoreMessageService::Push(const Message& message)
{
    if (message.GetType() != DefaultMessageType::CORE)
        return true;

    auto payload = reinterpret_cast<const CorePayload*>(message.GetPayload());

    switch (payload->m_type)
    {
    case DefaultCoreMessageType::LOAD_NEW_GAME:
        return LoadNewGame(payload->m_data);
    case DefaultCoreMessageType::SAVE_GAME:
        return SaveGame(payload->m_data);
    case DefaultCoreMessageType::LOAD_SAVE:
        return LoadSaveGame(payload->m_data);
    case DefaultCoreMessageType::LOAD_STATE:
        return LoadState(payload->m_data, payload->m_saveStateNumber);
    case DefaultCoreMessageType::SAVE_STATE:
        return SaveState(payload->m_data, payload->m_saveStateNumber);
    case DefaultCoreMessageType::CHANGE_MODE:
        m_bus.SetMode(payload->m_mode);
        return true;
    case DefaultCoreMessageType::RESET:
        m_bus.Reset();
        return true;
    case DefaultCoreMessageType::CORE_ENABLE_AUDIO:
        m_bus.GetAPU().SetEnable(payload->m_enable);
        return true;
    }

    return true;
}

bool CoreMessageService::Pull(Message& message)
{
    if (message.GetType() != DefaultMessageType::CORE && message.GetType() != DefaultMessageType::DEBUG)
        return true;

    if (message.GetType() == DefaultMessageType::CORE)
    {
        auto payload = reinterpret_cast<CorePayload*>(message.GetPayload());

        switch (payload->m_type)
        {
        case DefaultCoreMessageType::GET_MODE:
            payload->m_mode = m_bus.GetMode();
            return true;
        }
    }
    else if (message.GetType() == DefaultMessageType::DEBUG)
    {
        auto payload = reinterpret_cast<DebugPayload*>(message.GetPayload());

        switch (payload->m_type)
        {
        case DefaultDebugMessageType::READ_NAMETABLES:
            m_bus.GetPPU().CompleteReadOfNameTables(payload->m_data);
            return true;
        }
    }

    return true;
}

bool CoreMessageService::LoadNewGame(const std::string& file)
{
    // First stop the game
    m_bus.Stop();

    // Try to save the previous game before loading
    SaveGame("");

    Common::Utils::FileReadVisitor visitor(file);

    if (!visitor.IsValid())
    {
        m_bus.Resume();
        return false;
    }

    auto cartridge = std::make_shared<NesEmulator::Cartridge>(visitor);

    // Insert a new cartridge also reset the bus
    m_bus.InsertCartridge(cartridge);

    // Try to load an existing save
    LoadSaveGame("");

    // When all is done, restart the game
    m_bus.Resume();

    return true;
}

bool CoreMessageService::SaveState(const std::string& file, int number)
{
    std::string finalFile = file;
    if (file.empty())
    {
        finalFile = GetSaveStateFile(m_exePath, number, GetCartridgeUniqueID(m_bus.GetCartridge())).string();
    }

    if (finalFile.empty())
        return true;

    CreateFolders(finalFile);

    Common::Utils::FileWriteVisitor visitor(finalFile);
    if (!visitor.IsValid())
    {
        std::cerr << "Couldn't open file " << finalFile << std::endl;
        return false;
    }

    m_bus.SerializeTo(visitor);

    return true;
}

bool CoreMessageService::LoadState(const std::string& file, int number)
{
    std::string finalFile = file;
    if (file.empty())
    {
        finalFile = GetSaveStateFile(m_exePath, number, GetCartridgeUniqueID(m_bus.GetCartridge())).string();
    }

    if (finalFile.empty())
        return true;

    Common::Utils::FileReadVisitor visitor(finalFile);
    if (!visitor.IsValid())
    {
        std::cerr << "Couldn't open file " << file << std::endl;
        return false;
    }

    // When loading a state, we will reset the game then load
    // To start from a clean state.
    // To avoid multi threading issues, we stop the bus while doing so.
    // We don't want the bus to be clocked between a Reset and the deserialization
    m_bus.Stop();

    m_bus.Reset();
    m_bus.DeserializeFrom(visitor);

    m_bus.Resume();

    return true;
}

bool CoreMessageService::SaveGame(const std::string& file)
{
    const NesEmulator::Cartridge* cartridge = m_bus.GetCartridge();
    if (cartridge == nullptr || !cartridge->HasPersistantMemory())
        return true;

    std::string finalFile = file;
    if (file.empty())
    {
        finalFile = GetSaveFile(m_exePath, GetCartridgeUniqueID(cartridge)).string();
    }

    // Nothing to do
    if (finalFile.empty())
        return true;

    CreateFolders(finalFile);

    Common::Utils::FileWriteVisitor visitor(finalFile);
    if (!visitor.IsValid())
    {
        std::cerr << "Couldn't open file " << finalFile << std::endl;
        return false;
    }

    m_bus.SaveRAM(visitor);
    return true;
}

bool CoreMessageService::LoadSaveGame(const std::string& file)
{
    const NesEmulator::Cartridge* cartridge = m_bus.GetCartridge();
    if (cartridge == nullptr || !cartridge->HasPersistantMemory())
        return true;

    std::string finalFile = file;
    if (file.empty())
    {
        finalFile = GetSaveFile(m_exePath, GetCartridgeUniqueID(cartridge)).string();
    }

    if (finalFile.empty())
        return true;

    Common::Utils::FileReadVisitor visitor(finalFile);
    if (!visitor.IsValid())
    {
        std::cerr << "Couldn't open file " << file << std::endl;
        return false;
    }

    // Loading the RAM is guarded by a lock, so there is no need to stop
    // the game.
    m_bus.LoadRAM(visitor);
    return true;
}