#include "GLFW/glfw3.h"
#include "frontend/messageService/messageService.h"
#include "frontend/messageService/messages/debugMessage.h"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <core/constants.h>
#include <frontend/windows/nameTableWindow.h>
#include <imgui.h>

using NesEmulatorGL::NameTableWindow;
using NesEmulatorGL::Window;

NameTableWindow::NameTableWindow(unsigned width, unsigned height)
    : Window("Nametables", width, height)
{
    GLFWwindow* previousContextGLFW = glfwGetCurrentContext();
    glfwMakeContextCurrent(m_window);
    m_image = std::make_unique<Image>(NesEmulator::Cst::SCREEN_WIDTH * 2, NesEmulator::Cst::SCREEN_HEIGHT * 2);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGuiContext* previousContext = ImGui::GetCurrentContext();
    m_context = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_context);
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    if (previousContext != nullptr)
        ImGui::SetCurrentContext(previousContext);

    if (previousContextGLFW != nullptr)
        glfwMakeContextCurrent(previousContextGLFW);
}

NameTableWindow::~NameTableWindow()
{
    // Cleanup
    ImGui::DestroyContext(m_context);

    GLFWwindow* previousContextGLFW = glfwGetCurrentContext();
    glfwMakeContextCurrent(m_window);

    m_image.reset();

    if (previousContextGLFW != nullptr)
        glfwMakeContextCurrent(previousContextGLFW);
}

void NameTableWindow::InternalUpdate(bool externalSync)
{
    // ImGui::SetCurrentContext(m_context);

    // ImGui_ImplOpenGL3_NewFrame();
    // ImGui_ImplGlfw_NewFrame();
    // ImGui::NewFrame();

    // Get the data from the bus
    GetNametablesMessage message(m_image->GetInternalBuffer().data(), m_image->GetInternalBuffer().size(),
                                 m_image->GetInternalBuffer().size());
    if (DispatchMessageServiceSingleton::GetInstance().Pull(message))
    {
        m_image->UpdateGLTexture();
    }

    m_image->Draw();

    // ImGui::Render();
    // ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    // ImGui::EndFrame();
}
