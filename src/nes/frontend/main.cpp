#include "core/constants.h"
#include <algorithm>
#include <chrono>
#include <common/utils/fileVisitor.h>
#include <common/utils/utils.h>
#include <common/utils/vectorVisitor.h>
#include <core/bus.h>
#include <core/cartridge.h>
#include <filesystem>
#include <frontend/audio/nesAudioSystem.h>
#include <frontend/mainWindow.h>
#include <frontend/messageService/coreMessageService.h>
#include <frontend/messageService/messageService.h>
#include <frontend/messageService/messages/coreMessage.h>
#include <frontend/messageService/messages/screenMessage.h>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;
using namespace NesEmulatorGL;

static bool enableAudioByDefault = true;

int main(int argc, char** argv)
{
    // Load a rom from a file
    auto root = Common::Utils::GetRootPath();

    // Mapper 000 also
    auto path = root / "tests" / "nes" / "test_roms" / "nestest.nes";

    // path = root / "roms" / "smb.nes";

    // Check the arg, if there is a file to load
    if (argc > 1)
    {
        path = fs::path(argv[1]);
        if (path.is_relative())
            path = root / path;
    }

    NesEmulator::Bus bus;
    bus.GetAPU().SetEnable(enableAudioByDefault);

    // Create the core message service and connect it
    DispatchMessageServiceSingleton& singleton = DispatchMessageServiceSingleton::GetInstance();
    CoreMessageService messageService(bus, Common::Utils::GetExePath().string());
    singleton.Connect(&messageService);

    NesAudioSystem audioSystem(bus, false, 2, 256);
    audioSystem.Enable(enableAudioByDefault);

    // Load a new game
    singleton.Push(LoadNewGameMessage(path.string()));

    auto previous_point = std::chrono::high_resolution_clock::now();
    double extraClock = 0;
    constexpr bool showRealFPS = false;
    constexpr size_t nbSamples = 120;
    std::array<float, nbSamples> timeCounter;
    size_t ptr = 0;

    {
        NesEmulatorGL::MainWindow mainWindow("NES Emulator", 256 * 3, 240 * 3, bus.GetPPU().GetWidth(),
                                             bus.GetPPU().GetHeight());
        mainWindow.SetUserData(&bus);
        mainWindow.ConnectController();

        if (audioSystem.Initialize() || !enableAudioByDefault)
        {
            previous_point = std::chrono::high_resolution_clock::now();
            while (!mainWindow.RequestedClose())
            {
                auto start_point = std::chrono::high_resolution_clock::now();
                auto timeSpent =
                    std::chrono::duration_cast<std::chrono::microseconds>(start_point - previous_point).count();
                previous_point = std::chrono::high_resolution_clock::now();
                timeSpent = std::min<long long>(timeSpent, 16666ll);

                constexpr double cpuPeriodUS = 1000000.0 / NesEmulator::Cst::NTSC_CPU_FREQUENCY;
                double nbClocksDouble = (double)timeSpent / cpuPeriodUS;
                size_t nbClocks = (size_t)std::floor(nbClocksDouble);

                // The division won't have a well rounded number, so keep a counter of all the fractional parts of our
                // number of clocks and add one clock when it reaches 1.
                extraClock += nbClocksDouble - (double)nbClocks;
                while (extraClock >= 1.0)
                {
                    extraClock -= 1.0;
                    nbClocks++;
                }

                for (auto i = 0; i < nbClocks; ++i)
                {
                    if (bus.Clock())
                        DispatchMessageServiceSingleton::GetInstance().Push(RenderMessage(
                            bus.GetPPU().GetScreen(), bus.GetPPU().GetHeight() * bus.GetPPU().GetWidth()));
                }

                if constexpr (showRealFPS)
                {
                    auto end_point = std::chrono::high_resolution_clock::now();
                    timeSpent = std::chrono::duration_cast<std::chrono::microseconds>(end_point - start_point).count();
                    double ratio = (double)(timeSpent) / (cpuPeriodUS * nbClocks); // < 1 = faster than realtime
                    timeCounter[ptr++] = 60.0f / (float)ratio;
                    if (ptr == nbSamples)
                    {
                        ptr = 0;
                        float res = 0;
                        for (auto x : timeCounter)
                        {
                            res += x;
                        }
                        std::cout << "Real FPS: " << res / nbSamples << std::endl;
                    }
                }

                mainWindow.Update(true);
            }
        }

        bus.GetAPU().Stop();
        audioSystem.Shutdown();
    }

    // Save the game before closing
    singleton.Push(SaveGameMessage());

    // Cleanup
    singleton.Disconnect(&messageService);

    return 0;
}