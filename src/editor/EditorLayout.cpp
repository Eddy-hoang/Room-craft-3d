#include "roomcraft/editor/EditorLayout.hpp"

#include <algorithm>
#include <cmath>

namespace roomcraft::editor {

void EditorLayout::resetDefaults()
{
    m_leftWidth = 272.0f;
    m_rightWidth = 316.0f;
    m_hierarchyRatio = 0.43f;
    showLeftPanel = true;
    showRightPanel = true;
}

void EditorLayout::setLeftPanelWidth(float width)
{
    if (std::isfinite(width)) {
        m_leftWidth = std::clamp(width, kMinLeftWidth, kMaxLeftWidth);
    }
}

void EditorLayout::setRightPanelWidth(float width)
{
    if (std::isfinite(width)) {
        m_rightWidth = std::clamp(width, kMinRightWidth, kMaxRightWidth);
    }
}

void EditorLayout::setHierarchyHeightRatio(float ratio)
{
    if (std::isfinite(ratio)) {
        m_hierarchyRatio = std::clamp(ratio, kMinHierarchyRatio, kMaxHierarchyRatio);
    }
}

void EditorLayout::setUiScale(float scale)
{
    if (std::isfinite(scale)) {
        m_uiScale = std::clamp(scale, 1.0f, 1.50f);
    }
}

ComputedWorkspaceLayout EditorLayout::compute(float windowWidth,
                                              float windowHeight,
                                              float menuBarHeight) const
{
    ComputedWorkspaceLayout out;
    const float w = std::max(windowWidth, 640.0f);
    const float h = std::max(windowHeight, 480.0f);

    const float toolbarH = std::round(42.0f * m_uiScale);
    const float statusH = std::round(28.0f * m_uiScale);
    const float contentY = menuBarHeight + toolbarH;
    const float contentH = std::max(220.0f, h - contentY - statusH);

    out.toolbar = {0.0f, menuBarHeight, w, toolbarH};
    out.statusBar = {0.0f, contentY + contentH, w, statusH};

    // Tính bề rộng hai panel hai bên theo DPI nhưng LUÔN bảo đảm viewport trung tâm >= kMinCenterViewportWidth
    float leftW = showLeftPanel ? std::clamp(m_leftWidth * m_uiScale, kMinLeftWidth, kMaxLeftWidth) : 0.0f;
    float rightW = showRightPanel ? std::clamp(m_rightWidth * m_uiScale, kMinRightWidth, kMaxRightWidth) : 0.0f;

    const float splittersW = (showLeftPanel ? kSplitterThickness : 0.0f) +
                             (showRightPanel ? kSplitterThickness : 0.0f);
    const float maxCombinedSideW = std::max(0.0f, w - kMinCenterViewportWidth - splittersW);

    if (leftW + rightW > maxCombinedSideW && (leftW + rightW) > 1.0f) {
        const float shrink = maxCombinedSideW / (leftW + rightW);
        leftW = std::floor(leftW * shrink);
        rightW = std::floor(rightW * shrink);
    }

    // Nếu cửa sổ quá hẹp khiến panel trái dưới 160px -> tự động thu gọn để giữ viewport sử dụng được
    out.leftPanelVisible = showLeftPanel && (leftW >= 160.0f);
    out.rightPanelVisible = showRightPanel && (rightW >= 180.0f);
    if (!out.leftPanelVisible) leftW = 0.0f;
    if (!out.rightPanelVisible) rightW = 0.0f;

    float cursorX = 0.0f;
    if (out.leftPanelVisible) {
        out.leftPanel = {cursorX, contentY, leftW, contentH};
        cursorX += leftW;
        out.leftSplitter = {cursorX, contentY, kSplitterThickness, contentH};
        cursorX += kSplitterThickness;
    }

    const float rightReserved = out.rightPanelVisible ? (rightW + kSplitterThickness) : 0.0f;
    const float centerW = std::max(240.0f, w - cursorX - rightReserved);
    out.centerViewport = {cursorX, contentY, centerW, contentH};
    cursorX += centerW;

    if (out.rightPanelVisible) {
        out.rightSplitter = {cursorX, contentY, kSplitterThickness, contentH};
        cursorX += kSplitterThickness;

        const float availRightH = std::max(160.0f, contentH - kSplitterThickness);
        const float topH = std::clamp(std::floor(availRightH * m_hierarchyRatio), 110.0f, availRightH - 120.0f);
        const float bottomH = std::max(100.0f, availRightH - topH);

        out.rightTopPanel = {cursorX, contentY, rightW, topH};
        out.rightHorizSplitter = {cursorX, contentY + topH, rightW, kSplitterThickness};
        out.rightBottomPanel = {cursorX, contentY + topH + kSplitterThickness, rightW, bottomH};
    }

    return out;
}

} // namespace roomcraft::editor
