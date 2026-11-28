#pragma once

namespace roomcraft::editor {

struct PanelRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool isValid() const { return width > 1.0f && height > 1.0f; }
};

struct ComputedWorkspaceLayout {
    PanelRect toolbar;
    PanelRect leftPanel;          // Furniture Library
    PanelRect leftSplitter;
    PanelRect centerViewport;     // 3D Room Viewport (vùng trọng tâm lớn nhất)
    PanelRect rightSplitter;
    PanelRect rightTopPanel;      // Scene Hierarchy
    PanelRect rightHorizSplitter;
    PanelRect rightBottomPanel;   // Properties Inspector / Room Settings
    PanelRect statusBar;

    bool leftPanelVisible = true;
    bool rightPanelVisible = true;
};

// Quản lý bố cục không gian làm việc thích nghi nhiều độ phân giải (1280x720, 1366x768, 1600x900, 1920x1080)
// và tỷ lệ DPI (100%, 125%, 150%) với các thanh chia (splitter) kéo đổi kích thước mượt mà.
// Thuần CPU để kiểm thử tự động các ràng buộc không chồng lấn & không làm hẹp viewport.
class EditorLayout {
public:
    static constexpr float kSplitterThickness = 5.0f;
    static constexpr float kMinCenterViewportWidth = 380.0f;
    static constexpr float kMinLeftWidth = 220.0f;
    static constexpr float kMaxLeftWidth = 420.0f;
    static constexpr float kMinRightWidth = 260.0f;
    static constexpr float kMaxRightWidth = 460.0f;
    static constexpr float kMinHierarchyRatio = 0.25f;
    static constexpr float kMaxHierarchyRatio = 0.72f;

    EditorLayout() = default;

    void resetDefaults();

    void setLeftPanelWidth(float width);
    void setRightPanelWidth(float width);
    void setHierarchyHeightRatio(float ratio);
    void setUiScale(float scale);

    float getLeftPanelWidth() const { return m_leftWidth; }
    float getRightPanelWidth() const { return m_rightWidth; }
    float getHierarchyHeightRatio() const { return m_hierarchyRatio; }
    float getUiScale() const { return m_uiScale; }

    bool showLeftPanel = true;
    bool showRightPanel = true;

    ComputedWorkspaceLayout compute(float windowWidth,
                                    float windowHeight,
                                    float menuBarHeight) const;

private:
    float m_leftWidth = 272.0f;
    float m_rightWidth = 316.0f;
    float m_hierarchyRatio = 0.36f;
    float m_uiScale = 1.0f;
};

} // namespace roomcraft::editor
