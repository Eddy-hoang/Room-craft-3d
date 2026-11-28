#pragma once

#include <imgui.h>

#include <filesystem>
#include <string>

namespace roomcraft::editor {

// Tập Design Tokens thống nhất của RoomCraft 3D (Modern Dark CAD Theme — M6).
// Tuyệt đối không hardcode màu sắc hay spacing rải rác trong các panel giao diện.
namespace tokens {

inline constexpr ImVec4 kBg{0.090f, 0.102f, 0.125f, 1.0f};            // #171A20
inline constexpr ImVec4 kSurface{0.125f, 0.141f, 0.169f, 1.0f};       // #20242B
inline constexpr ImVec4 kPanel{0.161f, 0.180f, 0.220f, 1.0f};         // #292E38
inline constexpr ImVec4 kPanelHover{0.204f, 0.231f, 0.282f, 1.0f};    // #343B48
inline constexpr ImVec4 kPrimary{0.475f, 0.612f, 0.890f, 1.0f};       // #799CE3
inline constexpr ImVec4 kPrimaryHover{0.545f, 0.675f, 0.925f, 1.0f};
inline constexpr ImVec4 kPrimaryActive{0.388f, 0.525f, 0.812f, 1.0f};
inline constexpr ImVec4 kTextPrimary{0.902f, 0.918f, 0.949f, 1.0f};   // #E6EAF2
inline constexpr ImVec4 kTextSecondary{0.573f, 0.624f, 0.698f, 1.0f}; // #929FB2
inline constexpr ImVec4 kBorder{0.227f, 0.259f, 0.310f, 1.0f};        // #3A424F
inline constexpr ImVec4 kSelected{0.251f, 0.341f, 0.486f, 1.0f};      // #40577C
inline constexpr ImVec4 kSuccess{0.475f, 0.714f, 0.604f, 1.0f};       // #79B69A
inline constexpr ImVec4 kWarning{0.902f, 0.682f, 0.365f, 1.0f};       // #E6AE5D
inline constexpr ImVec4 kError{0.875f, 0.471f, 0.471f, 1.0f};         // #DF7878

// Màu trục tọa độ thống nhất trên toàn ứng dụng (Transform Input, Viewport Axes, Move Gizmo)
inline constexpr ImVec4 kAxisX{0.890f, 0.360f, 0.360f, 1.0f};         // Red
inline constexpr ImVec4 kAxisY{0.370f, 0.790f, 0.480f, 1.0f};         // Green
inline constexpr ImVec4 kAxisZ{0.360f, 0.610f, 0.920f, 1.0f};         // Blue

// Spacing scale (px ở DPI 100%)
inline constexpr float kSpaceXs = 4.0f;
inline constexpr float kSpaceSm = 8.0f;
inline constexpr float kSpaceMd = 12.0f;
inline constexpr float kSpaceLg = 16.0f;
inline constexpr float kSpaceXl = 24.0f;

inline constexpr float kWindowRounding = 5.0f;
inline constexpr float kFrameRounding = 5.0f;
inline constexpr float kCardRounding = 6.0f;

} // namespace tokens

struct FontLoadInfo {
    bool loadedCustomFont = false;
    std::string fontName = "ImGui Default (ProggyClean)";
    std::string resolvedPath;
    float baseSizePx = 15.0f;
    float dpiScale = 1.0f;
    ImFont* regularFont = nullptr;
    ImFont* headingFont = nullptr;
    ImFont* smallFont = nullptr;
};

class EditorTheme {
public:
    // Áp dụng bảng màu Modern Dark CAD và spacing tokens vào ImGuiStyle theo tỷ lệ DPI
    static void applyStyle(float uiScale = 1.0f);

    // Nạp font TrueType tương đối (Roboto-Medium.ttf, Apache 2.0) kèm fallback an toàn về font mặc định
    static FontLoadInfo loadFonts(const std::filesystem::path& fontFilePath,
                                  float baseSizePx = 15.0f,
                                  float uiScale = 1.0f);

    // Chuyển ImVec4 sang ImU32 phục vụ ImDrawList
    static ImU32 toU32(const ImVec4& color, float alphaOverride = -1.0f);
};

} // namespace roomcraft::editor
