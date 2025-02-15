#pragma once

#include <frontend/imguiWindows/imguiWindow.h>
#include <frontend/rendering/image.h>
#include <frontend/window.h>
#include <memory>
#include <string>

struct ImGuiContext;

namespace GBEmulatorExe
{
class TileDataWindow : public ImGuiWindow
{
public:
    TileDataWindow();

    WINDOW_ID_IMPL(AllWindowsId::TileDataWindowId);

protected:
    void UpdateImage();
    void DrawInternal() override;
    const char* GetWindowName() const override { return "TileData##12"; }

    double m_lastUpdateTime;
    bool m_forceUpdate = false;

    std::vector<uint8_t> m_data;
    unsigned m_currentBlock = 0;
    unsigned m_currentBank = 0;
    std::unique_ptr<Image> m_image;
    unsigned m_texture;
};
} // namespace GBEmulatorExe