#include "GLFW/glfw3.h"
#include "frontend/messageService/messages/debugMessage.h"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <core/utils/tile.h>
#include <core/utils/utils.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <frontend/imguiWindows/tileDataWindow.h>
#include <frontend/messageService/messageService.h>
#include <imgui.h>
#include <iostream>
#include <sstream>
#include <string>

using GBEmulatorExe::TileDataWindow;

TileDataWindow::TileDataWindow()
{
    m_lastUpdateTime = ImGui::GetTime();
    m_forceUpdate = true;
    m_data.resize(128 * 16 * 2);

    m_image = std::make_unique<Image>(16 * 8, 16 * 8); // 16 * 16 tiles
}

void TileDataWindow::UpdateImage()
{
    std::vector<uint8_t>& finalImage = m_image->GetInternalBuffer();

    std::array<uint8_t, 12> paletteColors;
    int i = 0;
    for (auto& color :
         {GBEmulator::WHITE_COLOR, GBEmulator::LIGHT_GREY_COLOR, GBEmulator::DARK_GREY_COLOR, GBEmulator::BLACK_COLOR})
    {
        GBEmulator::Utils::RGB555ToRGB888(color, paletteColors[i], paletteColors[i + 1], paletteColors[i + 2]);
        i += 3;
    }

    for (auto i = 0; i < 256; ++i)
    {
        std::array<uint8_t, 64> paletteColorIndexes = GBEmulator::Utils::GetTileDataFromBytes(m_data.data() + i * 16);
        int tileLine = i / 16;
        int tileColumn = i % 16;

        for (auto j = 0; j < 64; ++j)
        {
            int line = j / 8;
            int column = 7 - j % 8;

            int finalLine = tileLine * 8 + line;
            int finalColumn = tileColumn * 8 + column;

            std::memcpy(finalImage.data() + (finalLine * 128 + finalColumn) * 3,
                        paletteColors.data() + 3 * paletteColorIndexes[j], 3);
        }
    }

    m_image->UpdateGLTexture(true);
}

void TileDataWindow::DrawInternal()
{
    double elapsedTime = ImGui::GetTime();
    constexpr double updateDeltaTime = 0.1;
    if (elapsedTime > m_lastUpdateTime + updateDeltaTime || m_forceUpdate)
    {
        m_lastUpdateTime = elapsedTime;
        m_forceUpdate = false;
        GetVRAMMessage msg(m_data.data(), m_data.size(), m_data.size(), 0x8000 + m_currentBlock * 0x0800,
                           m_currentBank);
        DispatchMessageServiceSingleton::GetInstance().Pull(msg);

        UpdateImage();
    }

    static std::array<bool, 2> m_checkBoxesBlocks = {true, false};
    static std::array<bool, 2> m_checkBoxesBank = {true, false};

    ImGui::Text("%s", "Select a block: ");
    ImGui::SameLine();
    ImGui::Checkbox("Start address 0x8000", &m_checkBoxesBlocks[0]);
    ImGui::SameLine();
    ImGui::Checkbox("Start address 0x8800", &m_checkBoxesBlocks[1]);

    ImGui::Text("%s", "VRAM bank: ");
    ImGui::SameLine();
    ImGui::Checkbox("0", &m_checkBoxesBank[0]);
    ImGui::SameLine();
    ImGui::Checkbox("1", &m_checkBoxesBank[1]);

    for (auto i = 0; i < 2; i++)
    {
        if (m_checkBoxesBlocks[i] && i != m_currentBlock)
        {
            m_checkBoxesBlocks[m_currentBlock] = false;
            m_currentBlock = i;
        }

        if (m_checkBoxesBank[i] && i != m_currentBank)
        {
            m_checkBoxesBank[m_currentBank] = false;
            m_currentBank = i;
        }
    }

    ImGui::Image((void*)(intptr_t)m_image->GetTextureId(), ImVec2(16 * 8 * 5, 16 * 8 * 5));
}
