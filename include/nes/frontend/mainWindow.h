#pragma once

#include <chrono>
#include <cmath>
#include <cstdint>
#include <frontend/imguiManager.h>
#include <frontend/screen.h>
#include <frontend/window.h>
#include <memory>
#include <string>
#include <vector>

struct GLFWwindow;

namespace NesEmulatorGL
{
class Controller;

class MainWindow : public Window
{
public:
    MainWindow(const char* name, unsigned width, unsigned height, unsigned internalResWidth, unsigned internalResHeight,
               int framerate = NTSC_FRAMERATE);
    ~MainWindow() = default;

    bool RequestedClose() override;
    void OnScreenResized(int width, int height) override;

    // Need to be done after setting a bus
    void ConnectController();

protected:
    void InternalUpdate(bool externalSync) override;

private:
    std::shared_ptr<Controller> m_controller = nullptr;

    std::unique_ptr<ImguiManager> m_imguiManager;
    std::unique_ptr<Screen> m_screen;
};
} // namespace NesEmulatorGL