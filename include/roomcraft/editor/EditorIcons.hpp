#pragma once

#include <imgui.h>

namespace roomcraft::editor {

// Tập biểu tượng vector đồng nhất của RoomCraft 3D vẽ trực tiếp bằng ImDrawList:
// - Không phụ thuộc glyph Unicode dễ vỡ thành ô vuông (tofu).
// - Sắc nét ở mọi tỷ lệ DPI (100%, 125%, 150%) và hoàn toàn do dự án sở hữu (MIT).
enum class EditorIcon {
    AppLogo = 0,
    NewRoom,
    Select,
    Move,
    Rotate,
    Scale,
    Snap,
    Focus,
    Duplicate,
    Delete,
    EyeOpen,
    EyeClosed,
    LockClosed,
    LockOpen,
    Search,
    Settings,
    Camera,
    Warning,
    Help,
    Grid,
    Wireframe,
    Preset
};

class EditorIcons {
public:
    // Vẽ biểu tượng vector tại tâm `center` với kích thước `size` và màu `color`
    static void draw(ImDrawList* drawList,
                     EditorIcon icon,
                     ImVec2 center,
                     float size,
                     ImU32 color);

    // Nút công cụ thanh Toolbar kiểu CAD: icon vector phía trên + nhãn + phím tắt (hoặc ngang gọn)
    static bool toolbarButton(const char* id,
                              EditorIcon icon,
                              const char* label,
                              const char* shortcut,
                              bool active,
                              const char* tooltip,
                              bool compactHorizontal = true);

    // Nút icon nhỏ kèm tooltip bắt buộc (cho Scene Hierarchy: Eye, Lock, Duplicate, Delete)
    static bool smallIconButton(const char* id,
                                EditorIcon icon,
                                bool highlighted,
                                const char* tooltip,
                                ImVec4 iconTint = ImVec4(0.85f, 0.88f, 0.94f, 1.0f));
};

} // namespace roomcraft::editor
