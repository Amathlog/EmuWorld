#pragma once

#include <frontend/rendering/image.h>
#include <frontend/window.h>
#include <memory>

struct ImGuiContext;

namespace NesEmulatorGL
{
class NameTableWindow : public Window
{
public:
    NameTableWindow(unsigned width, unsigned height);
    ~NameTableWindow();

protected:
    void InternalUpdate(bool externalSync) override;
    ImGuiContext* m_context;
    std::unique_ptr<Image> m_image;
};
} // namespace NesEmulatorGL