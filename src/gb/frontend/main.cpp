#include <common/utils/fileVisitor.h>
#include <common/utils/utils.h>
#include <core/apu.h>
#include <core/bus.h>
#include <core/cartridge.h>
#include <core/constants.h>

#include <frontend/audio/gbAudioSystem.h>
#include <frontend/mainWindow.h>
#include <frontend/messageService/coreMessageService.h>
#include <frontend/messageService/messageService.h>
#include <frontend/messageService/messages/coreMessage.h>
#include <frontend/messageService/messages/screenMessage.h>
#include <frontend/messageService/messages/screenPayload.h>

#include <algorithm>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
using namespace GBEmulatorExe;

static bool enableAudioByDefault = true;
static bool syncWithAudio = false;

static unsigned windowScalingFactor = 5;

int main(int argc, char** argv)
{
    // Load a rom from a file
    auto root = Common::Utils::GetRootPath();
    auto path = std::filesystem::path();

    // Mapper 000 also
    path = root / "tests" / "gb" / "external_roms" / "dmg_sound" / "rom_singles" / "04-sweep.gb";

    // path = root / "roms" / "SuperMarioLand.gb";

    // Check the arg, if there is a file to load
    if (argc > 1)
    {
        path = fs::path(argv[1]);
        if (path.is_relative())
            path = root / path;
    }

    GBEmulator::Bus bus;

    GBAudioSystem audioSystem(bus, syncWithAudio, 2, GBEmulator::APU_SAMPLE_RATE, 256);
    audioSystem.Enable(enableAudioByDefault);

    GBEmulatorExe::CoreMessageService coreMessageService(bus, Common::Utils::GetExePath().string());
    GBEmulatorExe::DispatchMessageServiceSingleton::GetInstance().Connect(&coreMessageService);

    if (!path.empty())
    {
        LoadNewGameMessage msg(path.string());
        GBEmulatorExe::DispatchMessageServiceSingleton::GetInstance().Push(msg);
    }

    auto previous_point = std::chrono::high_resolution_clock::now();
    constexpr bool showRealFPS = false;
    constexpr size_t nbSamples = 120;
    std::array<float, nbSamples> timeCounter;
    size_t ptr = 0;

    {
        MainWindow mainWindow("GB/GBC Emulator", GBEmulator::GB_INTERNAL_WIDTH * windowScalingFactor,
                              GBEmulator::GB_INTERNAL_HEIGHT * windowScalingFactor, GBEmulator::GB_INTERNAL_WIDTH,
                              GBEmulator::GB_INTERNAL_HEIGHT);

        mainWindow.SetUserData(&bus);
        mainWindow.ConnectController();

        if (mainWindow.GetImguiManager()->IsBreakOnStart())
        {
            bus.BreakContinue();
        }

        if (audioSystem.Initialize() || !enableAudioByDefault)
        {
            previous_point = std::chrono::high_resolution_clock::now();
            while (!mainWindow.RequestedClose())
            {
                auto start_point = std::chrono::high_resolution_clock::now();
                auto timeSpent =
                    std::chrono::duration_cast<std::chrono::microseconds>(start_point - previous_point).count();
                previous_point = std::chrono::high_resolution_clock::now();
                // size_t nbInstructions = bus.GetCPU().GetNbInstructionsExecuted();
                // if (timeSpent > 16666ll)
                //{
                //     std::cout << "This frame took longer: " << timeSpent << "ms; NbInstructions = " <<
                //     nbInstructions<< std::endl;

                //    const auto& OpcodeCount = bus.GetCPU().GetOpcodeCount();
                //    std::array<std::pair<uint8_t, size_t>, 5> worstOnes;
                //    worstOnes.fill({ 0, 0 });
                //    for (auto i = 0; i < OpcodeCount.size(); ++i)
                //    {
                //        bool cascade = false;
                //        size_t j = 0;

                //        for (j = 0; j < 5; ++j)
                //        {
                //            if (worstOnes[j].second < OpcodeCount[i])
                //            {
                //                cascade = true;
                //                break;
                //            }
                //        }

                //        if (cascade)
                //        {
                //            for (size_t k = 4; k > j; --k)
                //            {
                //                worstOnes[k] = worstOnes[k - 1];
                //            }

                //            worstOnes[j] = { i, OpcodeCount[i] };
                //        }
                //    }

                //    std::cout << "Worst ones:" << std::endl;
                //    for (int i = 0; i < 5; ++i)
                //    {
                //        std::cout << "\tOpcode: " << +worstOnes[i].first << " ; Count: " << worstOnes[i].second <<
                //        std::endl;
                //    }
                //}
                // const_cast<GBEmulator::Z80Processor&>(bus.GetCPU()).ResetInstructionCount();
                timeSpent = std::min<int64_t>(timeSpent, 16666ll);

                constexpr double cpuPeriodSingleSpeedUS = 4.0 * 1000000.0 / GBEmulator::CPU_SINGLE_SPEED_FREQ_D;
                constexpr double cpuPeriodDoubleSpeedUS = 4.0 * 1000000.0 / GBEmulator::CPU_DOUBLE_SPEED_FREQ_D;
                double cpuPeriodUS = bus.IsInDoubleSpeedMode() ? cpuPeriodDoubleSpeedUS : cpuPeriodSingleSpeedUS;
                size_t nbClocks = (size_t)(timeSpent / cpuPeriodUS);
                if (!bus.IsInBreak())
                {
                    for (auto i = 0; i < nbClocks; ++i)
                    {
                        if (bus.Clock())
                            DispatchMessageServiceSingleton::GetInstance().Push(
                                RenderMessage(bus.GetPPU().GetScreen().data(), bus.GetPPU().GetScreen().size()));

                        if (bus.IsInBreak())
                            break;
                    }
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
                        float min = 100000;
                        float max = -1;
                        for (auto x : timeCounter)
                        {
                            res += x;
                            if (x < min)
                            {
                                min = x;
                            }
                            if (x > max)
                            {
                                max = x;
                            }
                        }
                        std::cout << "Real FPS: " << res / nbSamples << "; Min: " << min << "; Max: " << max
                                  << std::endl;
                    }
                }

                mainWindow.Update(true);
            }
        }

        bus.GetAPU().Stop();
        audioSystem.Enable(false);
    }

    GBEmulatorExe::DispatchMessageServiceSingleton::GetInstance().Push(SaveGameMessage());

    return 0;
}