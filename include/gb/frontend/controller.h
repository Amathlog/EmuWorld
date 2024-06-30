#pragma once

#include "frontend/mainWindow.h"
#include <core/controller.h>
#include <cstdint>

struct GLFWwindow;

namespace GBEmulatorExe
{
class Controller : public GBEmulator::Controller
{
public:
    Controller(GLFWwindow* window);
    ~Controller();

    void Update();

private:
    GLFWwindow* m_window;
};
} // namespace GBEmulatorExe