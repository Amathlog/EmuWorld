#pragma once

#include <type_traits>
#include <vector>
#include <cstdint>
#include <chrono>
#include <cmath>
#include <string>
#include <memory>
#include <type_traits>

struct GLFWwindow;

namespace NesEmulatorGL
{
    constexpr int NTSC_FRAMERATE = 60;
    constexpr int PAL_FRAMERATE = 50;
    constexpr int UNCAPPED_FRAMERATE = -1;

    class Window
    {
    public:
        Window(const char* name, unsigned width, unsigned height, int framerate = NTSC_FRAMERATE);
        virtual ~Window();

        void Update(bool externalSync);
        GLFWwindow* GetWindow() {return m_window;}

        void Enable(bool enable) {m_enable = enable;}
        bool IsEnabled() {return m_enable;}

        void SetFramerate(int framerate) {
            m_framerate = framerate; 
            m_frametimeUS = m_framerate < 0 ? 0u : static_cast<int64_t>(std::floor(1000000.0/ m_framerate));

            for (auto& childWindow : m_childrenWindows)
                childWindow->SetFramerate(framerate);
        }
        int GetFramerate() const { return m_framerate; }
        int64_t GetFrametime() const { return m_frametimeUS; }

        void SetUserData(void* userData) { m_userData = userData; }

        template <typename T, typename... Args, typename = std::enable_if_t<std::is_base_of_v<Window, T>>>
        Window* CreateNewChildWindow(Args&& ...args)
        {
            m_childrenWindows.push_back(std::make_unique<T>(std::forward<Args>(args)...));
            return m_childrenWindows.back().get();
        }

        virtual bool RequestedClose();
        virtual void OnScreenResized(int width, int height) {};

    protected:
        virtual void InternalUpdate(bool /*externalSync*/) {}

        void* m_userData = nullptr;
        GLFWwindow* m_window = nullptr;
        std::vector<std::unique_ptr<Window>> m_childrenWindows;
        bool m_enable = true;
        bool m_isMainWindow = false;

        int m_framerate = -1;
        int64_t m_frametimeUS;

        std::chrono::time_point<std::chrono::high_resolution_clock> m_lastUpdateTime;
    };
}