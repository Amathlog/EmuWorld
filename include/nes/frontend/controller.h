#pragma once

#include "frontend/mainWindow.h"
#include <core/controller.h>
#include <cstdint>

struct GLFWwindow;

namespace NesEmulatorGL
{
class Controller : public NesEmulator::Controller
{
public:
    Controller(GLFWwindow* window, uint8_t controllerIndex);
    ~Controller();

    void Update();

private:
    GLFWwindow* m_window;
    uint8_t m_controllerIndex;
};
} // namespace NesEmulatorGL