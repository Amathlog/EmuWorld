#include <frontend/imguiWindows/imguiWindow.h>
#include <imgui.h>

using NesEmulatorGL::ImGuiWindow;

void ImGuiWindow::Draw()
{
    if (!m_open)
        return;

    if (ImGui::Begin(GetWindowName(), &m_open))
    {
        m_width = ImGui::GetWindowWidth();
        m_height = ImGui::GetWindowHeight();

        DrawInternal();
    }

    ImGui::End();
}