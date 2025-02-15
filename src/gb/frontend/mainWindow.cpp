#include <core/bus.h>
#include <cstdint>
#include <frontend/controller.h>
#include <frontend/imguiManager.h>
#include <frontend/mainWindow.h>
#include <frontend/screen.h>
#include <memory>

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <frontend/messageService/messageService.h>
#include <frontend/messageService/messages/screenMessage.h>
#include <iostream>
#include <thread>

using GBEmulatorExe::MainWindow;

namespace
{
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    // Notify the screen that we have resized
    GBEmulatorExe::DispatchMessageServiceSingleton::GetInstance().Push(GBEmulatorExe::ResizeMessage(width, height));
}
} // namespace

MainWindow::MainWindow(const char* name, unsigned width, unsigned height, unsigned internalResWidth,
                       unsigned internalResHeight)
    : Window(name, width, height)
{
    m_isMainWindow = true;

    if (!m_enable)
        // Something went wrong
        return;

    m_controller = std::make_shared<GBEmulatorExe::Controller>(m_window);

    m_screen = std::make_unique<Screen>(internalResWidth, internalResHeight);
    m_screen->OnScreenResized(width, height);
    m_imguiManager = std::make_unique<ImguiManager>(this);

    m_enable = m_screen->IsInitialized();
}

void MainWindow::InternalUpdate(bool externalSync)
{
    // if (m_userData == nullptr)
    //     // No user data, something is wrong
    //     return;

    // GBEmulator::Bus* bus = reinterpret_cast<GBEmulator::Bus*>(m_userData);

    m_controller->Update();

    static bool hasPressed = false;

    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        hasPressed = true;
    }
    else if (hasPressed && glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_RELEASE)
    {
        hasPressed = false;
        m_imguiManager->ToggleMainMenu();
    }

    // if (!externalSync)
    //     m_screen->GetImage().UpdateInternalBuffer(bus->GetPPU().GetScreen(), bus->GetPPU().GetHeight() *
    //     bus->GetPPU().GetWidth());

    m_screen->Update();
    m_imguiManager->Update();
}

bool MainWindow::RequestedClose()
{
    if (!m_enable || !m_window)
        return true;

    return m_imguiManager->ShouldClose() || glfwWindowShouldClose(m_window);
}

void MainWindow::ConnectController()
{
    if (m_userData == nullptr)
        // No user data, something is wrong
        return;

    GBEmulator::Bus* bus = reinterpret_cast<GBEmulator::Bus*>(m_userData);
    bus->ConnectController(m_controller);
}

void MainWindow::OnScreenResized(int width, int height)
{
    if (m_screen != nullptr)
    {
        m_screen->OnScreenResized(width, height);
    }
}
