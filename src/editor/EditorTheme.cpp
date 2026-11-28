#include "roomcraft/editor/EditorTheme.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

namespace roomcraft::editor {

ImU32 EditorTheme::toU32(const ImVec4& color, float alphaOverride)
{
    ImVec4 c = color;
    if (alphaOverride >= 0.0f) {
        c.w = std::clamp(alphaOverride, 0.0f, 1.0f);
    }
    return ImGui::ColorConvertFloat4ToU32(c);
}

void EditorTheme::applyStyle(float uiScale)
{
    const float scale = std::clamp(uiScale, 0.85f, 1.75f);

    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s = ImGuiStyle(); // Khởi tạo lại sạch trước khi áp dụng scale để tránh scale chồng nhiều lần
    ImGui::StyleColorsDark(&s);

    s.WindowRounding = tokens::kWindowRounding;
    s.ChildRounding = tokens::kCardRounding;
    s.FrameRounding = tokens::kFrameRounding;
    s.PopupRounding = tokens::kCardRounding;
    s.ScrollbarRounding = 6.0f;
    s.GrabRounding = 4.0f;
    s.TabRounding = 5.0f;

    s.WindowBorderSize = 1.0f;
    s.ChildBorderSize = 1.0f;
    s.PopupBorderSize = 1.0f;
    s.FrameBorderSize = 1.0f;

    s.WindowPadding = ImVec2(10.0f, 8.0f);
    s.FramePadding = ImVec2(tokens::kSpaceSm, 5.0f);
    s.ItemSpacing = ImVec2(tokens::kSpaceSm, 6.0f);
    s.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    s.IndentSpacing = 14.0f;
    s.ScrollbarSize = 10.0f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text] = tokens::kTextPrimary;
    c[ImGuiCol_TextDisabled] = tokens::kTextSecondary;
    c[ImGuiCol_WindowBg] = tokens::kSurface;
    c[ImGuiCol_ChildBg] = tokens::kBg;
    c[ImGuiCol_PopupBg] = ImVec4(0.145f, 0.165f, 0.200f, 0.98f);
    c[ImGuiCol_Border] = tokens::kBorder;
    c[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    c[ImGuiCol_FrameBg] = tokens::kBg;
    c[ImGuiCol_FrameBgHovered] = tokens::kPanel;
    c[ImGuiCol_FrameBgActive] = tokens::kPanelHover;

    c[ImGuiCol_TitleBg] = tokens::kSurface;
    c[ImGuiCol_TitleBgActive] = tokens::kPanel;
    c[ImGuiCol_TitleBgCollapsed] = tokens::kBg;

    c[ImGuiCol_MenuBarBg] = tokens::kBg;
    c[ImGuiCol_ScrollbarBg] = tokens::kBg;
    c[ImGuiCol_ScrollbarGrab] = tokens::kPanelHover;
    c[ImGuiCol_ScrollbarGrabHovered] = tokens::kBorder;
    c[ImGuiCol_ScrollbarGrabActive] = tokens::kPrimary;

    c[ImGuiCol_CheckMark] = tokens::kPrimary;
    c[ImGuiCol_SliderGrab] = tokens::kPrimary;
    c[ImGuiCol_SliderGrabActive] = tokens::kPrimaryHover;

    c[ImGuiCol_Button] = tokens::kPanel;
    c[ImGuiCol_ButtonHovered] = tokens::kPanelHover;
    c[ImGuiCol_ButtonActive] = tokens::kSelected;

    c[ImGuiCol_Header] = tokens::kPanel;
    c[ImGuiCol_HeaderHovered] = tokens::kPanelHover;
    c[ImGuiCol_HeaderActive] = tokens::kSelected;

    c[ImGuiCol_Separator] = tokens::kBorder;
    c[ImGuiCol_SeparatorHovered] = tokens::kPrimary;
    c[ImGuiCol_SeparatorActive] = tokens::kPrimaryHover;

    c[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.32f, 0.40f, 0.35f);
    c[ImGuiCol_ResizeGripHovered] = tokens::kPrimary;
    c[ImGuiCol_ResizeGripActive] = tokens::kPrimaryHover;

    c[ImGuiCol_Tab] = tokens::kSurface;
    c[ImGuiCol_TabHovered] = tokens::kPanelHover;
    c[ImGuiCol_TabSelected] = tokens::kSelected;
    c[ImGuiCol_TabDimmed] = tokens::kBg;
    c[ImGuiCol_TabDimmedSelected] = tokens::kPanel;

    c[ImGuiCol_NavCursor] = tokens::kPrimary;
    c[ImGuiCol_ModalWindowDimBg] = ImVec4(0.05f, 0.06f, 0.08f, 0.62f);

    if (std::fabs(scale - 1.0f) > 1e-3f) {
        s.ScaleAllSizes(scale);
    }
    ImGui::GetIO().FontGlobalScale = scale;
}

FontLoadInfo EditorTheme::loadFonts(const std::filesystem::path& fontFilePath,
                                    float baseSizePx,
                                    float uiScale)
{
    FontLoadInfo info;
    info.baseSizePx = std::clamp(baseSizePx, 12.0f, 22.0f);
    info.dpiScale = std::clamp(uiScale, 0.85f, 1.75f);

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    std::error_code ec;
    if (!fontFilePath.empty() && std::filesystem::exists(fontFilePath, ec) && !ec) {
        // Đọc tệp font qua std::ifstream(std::filesystem::path) để hỗ trợ tuyệt đối đường dẫn Unicode & khoảng trắng trên Windows
        std::ifstream fontStream(fontFilePath, std::ios::binary | std::ios::ate);
        if (fontStream.is_open()) {
            const std::streamsize byteSize = fontStream.tellg();
            if (byteSize > 0 && byteSize <= 32 * 1024 * 1024) {
                std::vector<unsigned char> fontBytes(static_cast<std::size_t>(byteSize));
                fontStream.seekg(0, std::ios::beg);
                if (fontStream.read(reinterpret_cast<char*>(fontBytes.data()), byteSize)) {
                    const auto makeOwnedCopy = [&]() -> void* {
                        void* buf = IM_ALLOC(fontBytes.size());
                        if (buf != nullptr) {
                            std::memcpy(buf, fontBytes.data(), fontBytes.size());
                        }
                        return buf;
                    };

                    ImFontConfig cfg;
                    cfg.OversampleH = 2;
                    cfg.OversampleV = 2;
                    cfg.PixelSnapH = true;
                    cfg.FontDataOwnedByAtlas = true;

                    // Nạp cùng dải Latin mở rộng + tiếng Việt khi font hỗ trợ (Roboto-Medium.ttf, Apache 2.0)
                    const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesVietnamese();
                    void* regBuf = makeOwnedCopy();
                    ImFont* regular = (regBuf != nullptr)
                                          ? io.Fonts->AddFontFromMemoryTTF(regBuf, static_cast<int>(fontBytes.size()),
                                                                           info.baseSizePx, &cfg, glyphRanges)
                                          : nullptr;
                    if (regular != nullptr) {
                        void* headBuf = makeOwnedCopy();
                        void* smallBuf = makeOwnedCopy();
                        info.regularFont = regular;
                        info.headingFont = (headBuf != nullptr)
                                               ? io.Fonts->AddFontFromMemoryTTF(headBuf, static_cast<int>(fontBytes.size()),
                                                                                info.baseSizePx + 2.0f, &cfg, glyphRanges)
                                               : regular;
                        info.smallFont = (smallBuf != nullptr)
                                             ? io.Fonts->AddFontFromMemoryTTF(smallBuf, static_cast<int>(fontBytes.size()),
                                                                              std::max(12.0f, info.baseSizePx - 2.0f), &cfg, glyphRanges)
                                             : regular;
                        info.loadedCustomFont = true;
                        const auto fnameU8 = fontFilePath.filename().u8string();
                        const auto fullU8 = fontFilePath.u8string();
                        info.fontName = std::string(fnameU8.begin(), fnameU8.end()) + " (Apache 2.0)";
                        info.resolvedPath = std::string(fullU8.begin(), fullU8.end());
                        io.FontDefault = regular;
                        std::cout << "[RoomCraft3D] Loaded UI font: " << info.resolvedPath
                                  << " (" << info.baseSizePx << " px)" << std::endl;
                        return info;
                    }
                }
            }
        }
    }

    // Fallback an toàn về font mặc định của Dear ImGui nếu thiếu file font trên đĩa
    ImFontConfig defCfg;
    defCfg.SizePixels = info.baseSizePx;
    info.regularFont = io.Fonts->AddFontDefault(&defCfg);
    info.headingFont = info.regularFont;
    info.smallFont = info.regularFont;
    io.FontDefault = info.regularFont;
    info.loadedCustomFont = false;
    info.fontName = "ImGui Default Fallback (ProggyClean)";
    const auto warnU8 = fontFilePath.u8string();
    std::cout << "[RoomCraft3D][WARN] Custom UI font not found at '" << std::string(warnU8.begin(), warnU8.end())
              << "'; using ImGui default fallback font." << std::endl;
    return info;
}

} // namespace roomcraft::editor
