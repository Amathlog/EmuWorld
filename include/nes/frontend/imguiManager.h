#pragma once

#include <array>
#include <core/constants.h>
#include <frontend/imguiWindows/imguiWindow.h>
#include <frontend/messageService/messages/screenPayload.h>
#include <map>
#include <string>

struct ImGuiContext;

namespace NesEmulatorGL
{
class Window;

class ImguiManager
{
public:
    ImguiManager(Window* window);
    ~ImguiManager();

    bool ShouldClose() const { return m_closeRequested; }
    void ToggleMainMenu() { m_showMainMenu = !m_showMainMenu; }

    NesEmulator::Mode GetCurrentMode() { return m_currentMode; }

    void Update();

private:
    void HandleFileExplorer();
    void HandlePerf(bool showFPS);
    void UpdateCurrentMode();

    Window* m_window;
    ImGuiContext* m_context;

    bool m_showFileExplorer = false;
    bool m_closeRequested = false;
    bool m_showMainMenu = true;
    bool m_isSoundEnabled = false;
    bool m_previousSoundState = false;

    Format m_currentFormat = Format::ORIGINAL;

    std::array<bool, (size_t)Format::COUNT> m_changeFormats;

    inline static constexpr unsigned MAX_SAVE_STATES = 4;
    std::array<bool, MAX_SAVE_STATES> m_requestSaveState;
    std::array<bool, MAX_SAVE_STATES> m_requestLoadState;

    std::array<bool, (unsigned)NesEmulator::Mode::COUNT> m_requestChangeMode;
    NesEmulator::Mode m_currentMode;

    using ChildWidgetMap = std::map<int, std::unique_ptr<ImGuiWindow>>;
    ChildWidgetMap m_childWidgets;
};
} // namespace NesEmulatorGL